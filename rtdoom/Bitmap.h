#pragma once

#include <typeinfo>

#if 0

namespace rtdoom
{
constexpr unsigned blog2(unsigned v) // Preferred (faster, 32-bit)
{
    // find the log base 2 of 32-bit v
    const int MultiplyDeBruijnBitPosition[32] = {0, 9,  1,  10, 13, 21, 2,  29, 11, 14, 16, 18, 22, 25, 3, 30,
                                                 8, 12, 20, 28, 15, 17, 24, 7,  19, 27, 23, 6,  26, 5,  4, 31};

    v |= v >> 1; // first round down to one less than a power of 2
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;

    return MultiplyDeBruijnBitPosition[(unsigned)(v * 0x07C4ACDDU) >> 27];
}

/**
 * This class aims to be a faster replacement for vector<bool> and vector<vector<bool>>
 */
template <
    typename TInt = int,
    size_t TSize = sizeof(TInt),
    size_t TSizeLog = blog2(TSize),
    typename = decltype(std::is_integral_v<TInt>)>
class Bitmap1D
{
protected:
    TInt* m_vector;
    size_t m_bit_count;
    size_t m_bit_offset;
    bool   m_is_view;

public:
    Bitmap1D(size_t bit_offset = 0ULL) :
        m_vector(nullptr),
        m_bit_offset(bit_offset & (TSize-1)),
        m_is_view(true)
    {
    }

    Bitmap1D(TInt* buffer, size_t bit_offset = 0ULL) :
        m_vector(buffer),
        m_bit_offset(bit_offset & (TSize-1)),
        m_is_view(true)
    {
    }

    Bitmap1D() :
        m_vector(nullptr),
        m_bit_offset(0),
        m_is_view(false)
    {
    }

    virtual ~Bitmap1D() {
        if(!m_is_view)
            delete[] m_vector;
    }

    Bitmap1D& operator=(Bitmap1D const& rhs) {
        m_vector = rhs.m_vector;
        m_bit_count = rhs.m_bit_count;
        m_bit_offset = rhs.m_bit_offset;
    }

    void resize(size_t num_bits)
    {
        if(m_vector) delete[] m_vector;
        m_bit_count = num_bits;
        m_vector = new TInt[m_bit_count? ((num_bits + (m_bit_offset > 0)) + (TSize-1)) >> TSizeLog : 1];
    }

    bool bit(size_t at)
    {
        at += m_bit_offset;
        int unit = at & (TSize - 1);
        return m_vector[at >> TSizeLog] & (1 << unit);
    }

    bool bit_set(size_t at, bool bit)
    {
        at += m_bit_offset;
        int unit = at & (TSize - 1);
        m_vector[at >> TSizeLog] |= (1 << unit);
        return bit;
    }

    void set_view(bool is_view) {
        m_is_view = is_view;
    }

    size_t bit_count() const
    {
        return m_bit_count;
    }

    struct BitIterator
    {
        friend Bitmap1D;

        size_t m_bit_cursor;
        TInt*  m_vector_ref;
        TInt   m_chunk;

        bool operator*() const {
            return m_chunk & (1 << (m_bit_cursor & (TSize - 1)));
        }

        BitIterator& operator++() {
            size_t old_unit = m_bit_cursor++ >> TSizeLog;
            if((m_bit_cursor >> TSizeLog) != old_unit)
                m_chunk = m_vector_ref[(m_bit_cursor & (TSize - 1)) >> TSizeLog];
            return *this;
        }

        BitIterator& operator++(int) {
            size_t old_unit = m_bit_cursor++ >> TSizeLog;
            if((m_bit_cursor >> TSizeLog) != old_unit)
                m_chunk = m_vector_ref[(m_bit_cursor & (TSize - 1)) >> TSizeLog];
            return *this;
        }

        bool operator==(const BitIterator& rhs) {
            return (m_vector_ref == rhs.m_vector_ref) && (m_bit_cursor == rhs.m_bit_cursor);
        }

    private:
        BitIterator(TInt* vec, size_t bc) :
            m_vector_ref(vec),
            m_bit_cursor(bc)
        {
        }
    };

    BitIterator begin() {
        return BitIterator(m_vector, 0);
    }

    BitIterator end() {
        return BitIterator(m_vector, m_bit_count);
    }
};

/**
 * This class aims to be a faster replacement for vector<vector<bool>>
 */
template <
    typename TInt = int,
    size_t TSize = sizeof(TInt),
    size_t TSizeLog = blog2(TSize),
    typename = decltype(std::is_integral_v<TInt>)>
class Bitmap2D : public Bitmap1D<TInt>
{
    size_t m_pitch;
public:

    void resize(size_t num_bits, size_t num_rows)
    {
        m_pitch = num_rows;
        Bitmap1D<TInt>::resize(num_bits * num_rows);
    }

    size_t bit_pitch() const {
        return m_pitch;
    }

    Bitmap1D<TInt> row(size_t i) const {
        return Bitmap1D<TInt>(m_vector, i * bit_count);
    }

    struct BitmapIterator
    {
        friend class Bitmap2D;

        size_t m_bit_pitch;
        size_t m_bit_cursor;
        TInt*  m_vector_ref;
        Bitmap1D<TInt> m_row;

        Bitmap1D<TInt> const& operator*() const
        {
            return m_row;
        }

        BitmapIterator& operator++()
        {
            size_t old_p_offset = m_bit_cursor++ / m_bit_pitch;
            if((m_bit_cursor / m_bit_pitch) != old_p_offset)
            {
                m_row = Bitmap1D<TInt>(m_vector_ref + (m_bit_cursor >> TSizeLog), m_bit_cursor);
                m_row.set_view(true);
            }
            return *this;
        }

        BitmapIterator& operator++(int)
        {
            size_t old_p_offset = m_bit_cursor++ / m_bit_pitch;
            if((m_bit_cursor / m_bit_pitch) != old_p_offset)
            {
                // bm1d is a view by default when constructed this way
                m_row = Bitmap1D<TInt>(m_vector_ref + (m_bit_cursor >> TSizeLog), m_bit_cursor);
            }
            return *this;
        }

        bool operator==(const BitmapIterator& rhs)
        {
            return (m_vector_ref == rhs.m_vector_ref) && (m_bit_cursor == rhs.m_bit_cursor);
        }

        bool operator!=(const BitmapIterator& rhs)
        {
            return !(*this == rhs);
        }

    private:
        BitmapIterator(TInt* vec, size_t p, size_t bc) : m_vector_ref(vec), m_bit_pitch(p), m_bit_cursor(bc) { }
    };

    BitmapIterator begin()
    {
        return BitmapIterator(m_vector, m_pitch, 0);
    }

    BitmapIterator end()
    {
        return BitmapIterator(m_vector, m_pitch, m_bit_count);
    }
};

}

#endif
