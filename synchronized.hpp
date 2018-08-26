#ifndef SYNCHRONIZED_HPP_
#define SYNCHRONIZED_HPP_

#include <mutex>
#include <shared_mutex>
#include <type_traits>
#include <utility>


template <typename T>
class synchronized;


/**
* @brief
*    Make a synchronized<T> using template deduction.
*/
template <typename T> synchronized<T>
make_synchronized(T&& value) {
  return synchronized<T>(std::forward<T>(value));
}

/**
* @brief
*    Provides straightforward thread-synchronized access to templated type.
*
* @note
*    While access to the immediate type is synchronized, this class does not
*  prevent non-synchronized access of pointer or reference members of the
*  template type. Furthermore, it does not magically prevent deadlock.
*
* @note
*    For applicable types, prefer std::atomic<T>.
*/
template <typename T>
class synchronized {
 public:
  using value_t = std::remove_reference_t<T>;

  synchronized(synchronized&&) = delete;
  synchronized(const synchronized&) = delete;
  synchronized& operator=(synchronized&&) = delete;
  synchronized& operator=(const synchronized&) = delete;

  template <typename... Args>
  explicit synchronized(Args&&... args)
    : value_(std::forward<Args>(args)...)
  {}

  /**
  * @brief
  *    Thread-sychronized get.
  */
  value_t get() const {
    read_lock l(mutex_);
    return value_;
  }

  /**
  * @brief
  *    Thread-sychronized set.
  */
  template <typename U>
  void set(U&& new_value) {
    write_lock l(mutex_);
    value_ = std::forward<U>(new_value);
  }

  /**
  * @brief
  *    Use the underlying value where get() would be less-trivial or
  *  otherwise unsuitable.
  *
  * @note
  *    Prefer get(), especially if this access takes a long time, as to not
  *  starve a writer thread or other reader threads.
  */
  template <typename Accessor>
  void use(const Accessor& access) const {
    read_lock l(mutex_);
    access(value_);
  }

  /**
  * @brief
  *    Alter (mutate) the underlying value where get() and set() would be
  *  less-trivial or otherwise unsuitable.
  *
  * @note
  *    Prefer the combination of get() then set(), especially if this access
  *  takes a long time, as to not starve other writer or reader threads.
  */
  template <typename Mutator>
  void alter(const Mutator& mutate) {
    write_lock l(mutex_);
    mutate(value_);
  }

 private:
  value_t value_;
  
  using mutex_t = std::shared_timed_mutex;
  using read_lock = std::shared_lock<mutex_t>;
  using write_lock = std::unique_lock<mutex_t>;
  mutable mutex_t mutex_{};
};


#endif // SYNCHRONIZED_HPP_
