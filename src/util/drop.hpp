namespace chatterino {

/**
 * Calls the destructor of var immediately.
 *
 * This is useful before noreturn functions to prevent leaking memory from complex types.
 */
template <typename T>
inline void drop(T &var)
{
    var.~T();
}

/**
 * Helps you avoid accidentally dropping a pointer not the object behind it.
 */
template <typename T>
inline void drop(T * /*var*/) = delete;

}  // namespace chatterino
