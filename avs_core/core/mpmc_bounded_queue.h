#include <boost/circular_buffer.hpp>
#include <mutex>
#include <condition_variable>

template <typename T>
class mpmc_bounded_queue
{
public:
  typedef boost::circular_buffer<T> container_type;
  typedef typename container_type::size_type size_type;
  typedef typename container_type::value_type value_type;

private:
  mpmc_bounded_queue(const mpmc_bounded_queue&);              // Disabled copy constructor
  mpmc_bounded_queue& operator = (const mpmc_bounded_queue&); // Disabled assign operator

  bool empty() const { return m_unread == 0; }
  bool full() const { return m_unread == m_container.capacity(); }

  size_type m_unread;
  container_type m_container;
  std::mutex m_mutex;
  std::condition_variable m_not_empty;
  std::condition_variable m_not_full;

public:
  mpmc_bounded_queue(size_type capacity) :
    m_unread(0), m_container(capacity)
  {
  }

  size_type capacity() const
  {
    return m_container.capacity();
  }

  void push_front(T const& item)
  {
    std::unique_lock<std::mutex> lock(m_mutex);
    while(full())
    {
      m_not_full.wait(lock);
    }
    m_container.push_front(item);
    ++m_unread;
    lock.unlock();
    m_not_empty.notify_one();
  }

  void pop_back(value_type* pItem)
  {
    std::unique_lock<std::mutex> lock(m_mutex);
    while(empty())
    {
      m_not_empty.wait(lock);
    }
    *pItem = std::move(m_container[--m_unread]);
    lock.unlock();
    m_not_full.notify_one();
  }
};
