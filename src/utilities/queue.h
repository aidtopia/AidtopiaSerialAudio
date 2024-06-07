#ifndef AIDTOPIA_SERIALAUDIOQUEUE_H
#define AIDTOPIA_SERIALAUDIOQUEUE_H

namespace aidtopia {

// A ring-buffer based queue with a maximum of 8 elements.
template <typename T, uint8_t CAPACITY=8>
class Queue {
    public:
        Queue() { clear(); }

        void clear() {
            m_head = m_tail = 0;
            m_full = 0;
            m_reserved = 0;
        }
        
        bool empty() const { return m_head == m_tail && !m_full; }
        bool full() const  { return m_full; }

        // Assumes !empty().
        T const &peekFront() const { return m_buffer[m_head]; }

        void popFront() {
            if (empty()) return;
            m_head = (m_head + 1) % CAPACITY;
            m_full = 0;
        }

        // Returns true if successful or false if the queue is already full.
        bool pushBack(T const &item) {
            if (m_full) return false;
            m_buffer[m_tail] = item;
            m_tail = (m_tail + 1) % CAPACITY;
            m_full = m_tail == m_head;
            return true;
        }

    private:
        static_assert(2 <= CAPACITY && CAPACITY <= 8);

        T       m_buffer[CAPACITY];
        uint8_t m_head : 3;  // log_2(CAPACITY)
        uint8_t m_tail : 3;  // log_2(CAPACITY)
        uint8_t m_full : 1;
        uint8_t m_reserved : 1;
};

}

#endif
