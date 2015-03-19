#include <vector>

template <typename T>
class circular_buffer
{
public:

  typedef size_t size_type;
  typedef T value_type;

private:
  std::vector<T>  array;
  const size_type max_size; 
  size_type head; 
  size_type tail; 
  size_type size;

public:

  circular_buffer(size_type capacity) :
    array(capacity),
    max_size(capacity),
    head(0),
    tail(0),
    size(0)
  {
  }

  bool empty() const
  {
    return size == 0;
  }

  bool full() const
  {
    assert(size <= max_size);
    return size == max_size;
  }

  bool push_front(const T& item)
  {
    if (full())
      return false;

    array[head] = item;
    head = ++head % max_size;
    ++size;

    return true;
  }

  bool pop_back(T* pItem)
  {
    if (empty())
      return false;

    *pItem = std::move(array[tail]);
    tail = ++tail % max_size;
    --size;

    return true;
  }

  size_type capacity() const
  {
    return max_size;
  }
};

#include <mutex>
#include <condition_variable>

template <typename T>
class mpmc_bounded_queue
{
public:
  typedef circular_buffer<T> container_type;
  typedef typename container_type::size_type size_type;
  typedef typename container_type::value_type value_type;

private:
  mpmc_bounded_queue(const mpmc_bounded_queue&);              // Disabled copy constructor
  mpmc_bounded_queue& operator = (const mpmc_bounded_queue&); // Disabled assign operator

  container_type m_container;
  std::mutex m_mutex;
  std::condition_variable m_not_empty;
  std::condition_variable m_not_full;

public:
  mpmc_bounded_queue(size_type capacity) :
    m_container(capacity)
  {
  }

  size_type capacity() const
  {
    return m_container.capacity();
  }

  void push_front(T const& item)
  {
    std::unique_lock<std::mutex> lock(m_mutex);
    while(m_container.full())
    {
      m_not_full.wait(lock);
    }
    m_container.push_front(item);
    lock.unlock();
    m_not_empty.notify_one();
  }

  void pop_back(value_type* pItem)
  {
    std::unique_lock<std::mutex> lock(m_mutex);
    while(m_container.empty())
    {
      m_not_empty.wait(lock);
    }
    m_container.pop_back(pItem);
    lock.unlock();
    m_not_full.notify_one();
  }
};
