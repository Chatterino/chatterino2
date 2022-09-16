#pragma once
#include <functional>

#include "common/NetworkCommon.hpp"
#include "common/NetworkRequest.hpp"
#include "common/NetworkResult.hpp"
#include "common/Outcome.hpp"

namespace chatterino {

// i can't make it not pass something, so this is a small type that does nothing
struct APIRequestNoErrorValue {
};
template <typename OkType, typename ErrorType>
class APIRequestData
{
    using APISuceessCallback = std::function<Outcome(NetworkResult, OkType)>;
    using APIErrorCallback = std::function<void(NetworkResult, ErrorType)>;
    using APIFinallyCallback = std::function<void()>;

public:
    APISuceessCallback onSuccess;
    APIErrorCallback onError;
    APIFinallyCallback finally;
};

template <typename OkType, typename ErrorType>
class APIRequest
{
    using APISuceessCallback = std::function<Outcome(NetworkResult, OkType)>;
    using APIErrorCallback = std::function<void(NetworkResult, ErrorType)>;
    using APIFinallyCallback = std::function<void()>;

private:
    NetworkRequest underlying_;
    std::shared_ptr<APIRequestData<OkType, ErrorType>> data_;

public:
    // move = good
    APIRequest(APIRequest<OkType, ErrorType> &&other) = default;
    APIRequest &operator=(APIRequest<OkType, ErrorType> &&other) = default;

    // copy = bad
    APIRequest(const APIRequest<OkType, ErrorType> &other) = delete;
    APIRequest &operator=(const APIRequest<OkType, ErrorType> &other) = delete;

    /**
     * @brief Constructs an APIRequest instance and a APIRequestData.
     *
     * @param[successTransformer] a function that will transform a NetworkResult into the desired type (OkType), for example decode JSON
     * @param[errorTransformer] an optional function that will transform a NetworkResult into the desired type (ErrorType)
     * @param[onFinally] an optional function that will be called regardless of the status. This is explicitly different from calling finally(...) as this is for cleanup of the decoding/transformer logic, they are separate
     *
     * APIRequest is a wrapper around NetworkRequest
     */
    explicit APIRequest(
        QUrl url, NetworkRequestType requestType,
        std::function<OkType(NetworkResult)> successTransformer,
        std::function<ErrorType(NetworkResult)> errorTransformer = nullptr,
        std::function<void()> onFinally = nullptr)
        : underlying_(std::move(url), requestType)
        , data_(std::make_shared<APIRequestData<OkType, ErrorType>>())
    {
        auto data = this->data_;
        this->underlying_ =
            std::move(this->underlying_)
                .timeout(5 * 1000)
                .onSuccess(
                    [data, successTransformer](NetworkResult res) -> Outcome {
                        OkType val = successTransformer(res);
                        return data->onSuccess(res, val);
                    })
                .onError([data, errorTransformer](NetworkResult res) {
                    if (errorTransformer)
                    {
                        ErrorType err = errorTransformer(res);
                        data->onError(res, err);
                    }
                })
                .finally([data, onFinally]() {
                    if (onFinally)
                    {
                        onFinally();
                    }
                    if (data->finally)
                    {
                        data->finally();
                    }
                });
    };

    APIRequest<OkType, ErrorType> header(const char *headerName,
                                         const char *value) &&
    {
        this->underlying_ =
            std::move(this->underlying_).header(headerName, value);
        return std::move(*this);
    }

    APIRequest<OkType, ErrorType> onError(APIErrorCallback cb) &&
    {
        this->data_->onError = cb;
        return std::move(*this);
    };

    APIRequest<OkType, ErrorType> onSuccess(APISuceessCallback cb) &&
    {
        this->data_->onSuccess = cb;
        return std::move(*this);
    };

    APIRequest<OkType, ErrorType> finally(APIFinallyCallback cb) &&
    {
        this->data_->finally = cb;
        return std::move(*this);
    };

    void execute()
    {
        this->underlying_.execute();
    }
};

}  // namespace chatterino
