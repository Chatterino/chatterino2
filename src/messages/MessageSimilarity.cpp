#include "messages/MessageSimilarity.hpp"

#include <algorithm>
#include <vector>

namespace chatterino::similarity::detail {

float relativeSimilarity(QStringView str1, QStringView str2)
{
    using SizeType = QStringView::size_type;

    // Longest Common Substring Problem
    std::vector<std::vector<int>> tree(str1.size(),
                                       std::vector<int>(str2.size(), 0));
    int z = 0;

    for (SizeType i = 0; i < str1.size(); ++i)
    {
        for (SizeType j = 0; j < str2.size(); ++j)
        {
            if (str1[i] == str2[j])
            {
                if (i == 0 || j == 0)
                {
                    tree[i][j] = 1;
                }
                else
                {
                    tree[i][j] = tree[i - 1][j - 1] + 1;
                }
                z = std::max(tree[i][j], z);
            }
            else
            {
                tree[i][j] = 0;
            }
        }
    }

    // ensure that no div by 0
    if (z == 0)
    {
        return 0.F;
    }

    auto div = std::max<>({static_cast<SizeType>(1), str1.size(), str2.size()});

    return float(z) / float(div);
}

}  // namespace chatterino::similarity::detail
