/*************************************************************************
* Name:        huffman.c
* Author:      Marcus Geelnard
* Description: Huffman coder/decoder implementation.
* Reentrant:   Yes
* $Id: huffman.c,v 1.1 2004/07/23 13:04:28 sh0dan Exp $
*
* This is a very straight forward implementation of a Huffman coder and
* decoder.
*
* Primary flaws with this primitive implementation are:
*  - Slow bit stream implementation
*  - Fairly slow decoding (slower than encoding)
*  - Maximum tree depth of 32 (the coder aborts if any code exceeds a
*    size of 32 bits). If I'm not mistaking, this should not be possible
*    unless the input buffer is larger than 2^32 bytes, which is not
*    supported by the coder anyway (max 2^32-1 bytes can be specified with
*    an unsigned 32-bit integer).
*
* On the other hand, there are a few advantages of this implementation:
*  - The Huffman tree is stored in a very compact form, requiring only
*    12 bits per symbol (for 8 bit symbols), meaning a maximum of 384
*    bytes overhead.
*  - The Huffman coder does quite well in situations where the data is
*    noisy, in which case most dictionary based coders run into problems.
*
* Possible improvements (probably not worth it):
*  - Partition the input data stream into blocks, where each block has
*    its own Huffman tree. With variable block sizes, it should be
*    possible to find locally optimal Huffman trees, which in turn could
*    reduce the total size.
*  - Allow for a few different predefined Huffman trees, which could
*    reduce the size of a block even further.
*-------------------------------------------------------------------------
* Copyright (c) 2003-2004 Marcus Geelnard
*
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
*
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
*
* 1. The origin of this software must not be misrepresented; you must not
*    claim that you wrote the original software. If you use this software
*    in a product, an acknowledgment in the product documentation would
*    be appreciated but is not required.
*
* 2. Altered source versions must be plainly marked as such, and must not
*    be misrepresented as being the original software.
*
* 3. This notice may not be removed or altered from any source
*    distribution.
*
* Marcus Geelnard
* marcus.geelnard at home.se
*************************************************************************/



/*************************************************************************
* Types used for Huffman coding
*************************************************************************/

typedef struct {
    unsigned int Symbol;
    unsigned int Count;
    unsigned int Code;
    unsigned int Bits;
} huff_sym_t;

typedef struct {
    unsigned char *BytePtr;
    unsigned int  BitPos;
    unsigned int  NumBytes;
} huff_bitstream_t;



/*************************************************************************
*                           INTERNAL FUNCTIONS                           *
*************************************************************************/


/*************************************************************************
* _Huffman_InitBitstream() - Initialize a bitstream.
*************************************************************************/

static void _Huffman_InitBitstream( huff_bitstream_t *stream,
    unsigned char *buf, unsigned int bytes )
{
    stream->BytePtr  = buf;
    stream->BitPos   = 0;
    stream->NumBytes = bytes;
}


/*************************************************************************
* _Huffman_ReadBits() - Read bits from a bitstream.
*************************************************************************/

static unsigned int _Huffman_ReadBits( huff_bitstream_t *stream,
    unsigned int bits )
{
    unsigned int  x, bit, bytesleft, count;
    unsigned char *buf;

    /* Get current stream state */
    buf       = stream->BytePtr;
    bit       = stream->BitPos;
    bytesleft = stream->NumBytes;

    /* Extract bits */
    x = 0;
    for( count = 0; count < bits && bytesleft >= 1; ++ count )
    {
        x = (x<<1) + (*buf & (1<<(7-bit)) ? 1 : 0);
        bit = (bit+1) & 7;
        if( !bit )
        {
            -- bytesleft;
            ++ buf;
        }
    }

    /* Store new stream state */
    stream->BytePtr  = buf;
    stream->BitPos   = bit;
    stream->NumBytes = bytesleft;

    return x;
}


/*************************************************************************
* _Huffman_WriteBits() - Write bits to a bitstream.
*************************************************************************/

static void _Huffman_WriteBits( huff_bitstream_t *stream, unsigned int x,
    unsigned int bits )
{
    unsigned int  bit, bytesleft, count;
    unsigned char *buf;
    unsigned int  mask;

    /* Get current stream state */
    buf       = stream->BytePtr;
    bit       = stream->BitPos;
    bytesleft = stream->NumBytes;

    /* Append bits */
    mask = 1 << (bits-1);
    for( count = 0; count < bits && bytesleft >= 1; ++ count )
    {
        *buf = (*buf & (0xff^(1<<(7-bit)))) +
               ((x & mask ? 1 : 0) << (7-bit));
        x <<= 1;
        bit = (bit+1) & 7;
        if( !bit )
        {
            -- bytesleft;
            ++ buf;
        }
    }

    /* Store new stream state */
    stream->BytePtr  = buf;
    stream->BitPos   = bit;
    stream->NumBytes = bytesleft;
}


/*************************************************************************
* _Huffman_Hist() - Calculate (sorted) histogram for a block of data.
*************************************************************************/

static void _Huffman_Hist( unsigned char *in, huff_sym_t *sym,
    unsigned int size )
{
    int k, swaps;
    huff_sym_t tmp;

    /* Clear/init histogram */
    for( k = 0; k < 256; ++ k )
    {
        sym[k].Symbol = k;
        sym[k].Count  = 0;
        sym[k].Code   = 0;
        sym[k].Bits   = 0;
    }

    /* Build histogram */
    for( k = size; k; -- k )
    {
        sym[ *in ++ ].Count ++;
    }

    /* Sort histogram - most frequent symbol first (bubble sort) */
    do
    {
        swaps = 0;
        for( k = 0; k < 255; ++ k )
        {
            if( sym[k].Count < sym[k+1].Count )
            {
                tmp      = sym[k];
                sym[k]   = sym[k+1];
                sym[k+1] = tmp;
                swaps    = 1;
            }
        }
    }
    while( swaps );
}


/*************************************************************************
* _Huffman_MakeTree() - Generate a Huffman tree.
*************************************************************************/

static void _Huffman_MakeTree( huff_sym_t *sym, huff_bitstream_t *stream,
    unsigned int code, unsigned int bits, unsigned int first,
    unsigned int last )
{
    unsigned int k, size, size_a, size_b, last_a, first_b;

    /* Is this a leaf node? */
    if( first == last )
    {
        /* Append symbol to tree description */
        _Huffman_WriteBits( stream, 1, 1 );
        _Huffman_WriteBits( stream, sym[first].Symbol, 8 );

        /* Store code info in symbol array */
        sym[first].Code = code;
        sym[first].Bits = bits;
        return;
    }
    else
    {
        /* This was not a leaf node */
        _Huffman_WriteBits( stream, 0, 1 );
    }

    /* Total size of interval */
    size = 0;
    for( k = first; k <= last; ++ k )
    {
        size += sym[k].Count;
    }

    /* Find size of branch a */
    size_a = 0;
    for( k = first; size_a <= size/2 && k < last; ++ k )
    {
        size_a += sym[k].Count;
    }

    /* Branch a and b cut in histogram */
    last_a  = k-1;
    first_b = k;

    /* Non-empty branch? */
    if( size_a > 0 )
    {
        /* Continue branching */
        _Huffman_WriteBits( stream, 1, 1 );

        /* Create branch a */
        _Huffman_MakeTree( sym, stream, (code<<1)+0, bits+1,
                               first, last_a );
    }
    else
    {
        /* This was an empty branch */
        _Huffman_WriteBits( stream, 0, 1 );
    }

    /* Size of branch b */
    size_b = size - size_a;

    /* Non-empty branch? */
    if( size_b > 0 )
    {
        /* Continue branching */
        _Huffman_WriteBits( stream, 1, 1 );

        /* Create branch b */
        _Huffman_MakeTree( sym, stream, (code<<1)+1, bits+1,
                               first_b, last );
    }
    else
    {
        /* This was an empty branch */
        _Huffman_WriteBits( stream, 0, 1 );
    }
}


/*************************************************************************
* _Huffman_RecoverTree() - Recover a Huffman tree from a bitstream.
*************************************************************************/

static void _Huffman_RecoverTree( huff_sym_t *sym,
    huff_bitstream_t *stream, unsigned int code, unsigned int bits,
    unsigned int *symnum )
{
    unsigned int symbol;

    /* Is this a leaf node? */
    if( _Huffman_ReadBits( stream, 1 ) )
    {
        /* Get symbol from tree description */
        symbol = _Huffman_ReadBits( stream, 8 );

        /* Store code info in symbol array */
        sym[*symnum].Symbol = symbol;
        sym[*symnum].Code   = code;
        sym[*symnum].Bits   = bits;

        /* Increase symbol counter */
        *symnum = *symnum + 1;

        return;
    }

    /* Non-empty branch? */
    if( _Huffman_ReadBits( stream, 1 ) )
    {
        /* Create branch a */
        _Huffman_RecoverTree( sym, stream, (code<<1)+0, bits+1,
                              symnum );
    }

    /* Non-empty branch? */
    if( _Huffman_ReadBits( stream, 1 ) )
    {
        /* Create branch b */
        _Huffman_RecoverTree( sym, stream, (code<<1)+1, bits+1,
                              symnum );
    }
}




/*************************************************************************
*                            PUBLIC FUNCTIONS                            *
*************************************************************************/


/*************************************************************************
* Huffman_Compress() - Compress a block of data using a Huffman coder.
*  in     - Input (uncompressed) buffer.
*  out    - Output (compressed) buffer. This buffer must be 384 bytes
*           larger than the input buffer.
*  insize - Number of input bytes.
* The function returns the size of the compressed data.
*************************************************************************/

int Huffman_Compress( unsigned char *in, unsigned char *out,
    unsigned int insize )
{
    huff_sym_t       sym[ 256 ], tmp;
    huff_bitstream_t stream;
    unsigned int     k, bufsize, total_bytes, swaps, symbol;

    /* Do we have anything to compress? */
    if( insize < 1 ) return 0;

    /* Calculate necessary buffer memory (we need max 384 bytes for
       storing the tree) */
    bufsize = insize + 384;

    /* Initialize bitstream */
    _Huffman_InitBitstream( &stream, out, bufsize );

    /* Calculate and sort histogram for input data */
    _Huffman_Hist( in, sym, insize );

    /* Build Huffman tree */
    _Huffman_MakeTree( sym, &stream, 0, 0, 0, 255 );

    /* Was any code > 32 bits? (we do not handle that at present) */
    for( k = 0; k < 255; ++ k )
    {
        if( sym[k].Bits > 32 )
        {
            return 0;
        }
    }

    /* Sort histogram - first symbol first (bubble sort) */
    do
    {
        swaps = 0;
        for( k = 0; k < 255; ++ k )
        {
            if( sym[k].Symbol > sym[k+1].Symbol )
            {
                tmp      = sym[k];
                sym[k]   = sym[k+1];
                sym[k+1] = tmp;
                swaps    = 1;
            }
        }
    }
    while( swaps );

    /* Encode input stream */
    for( k = 0; k < insize; ++ k )
    {
        symbol = in[ k ];
        _Huffman_WriteBits( &stream, sym[symbol].Code,
                            sym[symbol].Bits );
    }

    /* Calculate size of output data */
    total_bytes = (int)(stream.BytePtr - out);
    if( stream.BitPos > 0 )
    {
        ++ total_bytes;
    }

    return total_bytes;
}


/*************************************************************************
* Huffman_Uncompress() - Uncompress a block of data using a Huffman
* decoder.
*  in      - Input (compressed) buffer.
*  out     - Output (uncompressed) buffer. This buffer must be large
*            enough to hold the uncompressed data.
*  insize  - Number of input bytes.
*  outsize - Number of output bytes.
*************************************************************************/

void Huffman_Uncompress( unsigned char *in, unsigned char *out,
    unsigned int insize, unsigned int outsize )
{
    huff_sym_t       sym[ 256 ], tmp;
    huff_bitstream_t stream;
    unsigned int     k, m, symbol_count, swaps;
    unsigned char    *buf;
    unsigned int     bits, delta_bits, new_bits, code;

    /* Do we have anything to decompress? */
    if( insize < 1 ) return;

    /* Initialize bitstream */
    _Huffman_InitBitstream( &stream, in, insize );

    /* Clear tree/histogram */
    for( k = 0; k < 256; ++ k )
    {
        sym[k].Bits = 0x7fffffff;
    }

    /* Recover Huffman tree */
    symbol_count = 0;
    _Huffman_RecoverTree( sym, &stream, 0, 0, &symbol_count );

    /* Sort histogram - shortest code first (bubble sort) */
    do
    {
        swaps = 0;
        for( k = 0; k < symbol_count-1; ++ k )
        {
            if( sym[k].Bits > sym[k+1].Bits )
            {
                tmp      = sym[k];
                sym[k]   = sym[k+1];
                sym[k+1] = tmp;
                swaps    = 1;
            }
        }
    }
    while( swaps );

    /* Decode input stream */
    buf = out;
    for( k = 0; k < outsize; ++ k )
    {
        /* Search tree for matching code */
        bits = 0;
        code = 0;
        for( m = 0; m < symbol_count; ++ m )
        {
            delta_bits = sym[m].Bits - bits;
            if( delta_bits )
            {
                new_bits = _Huffman_ReadBits( &stream, delta_bits );
                code = code | (new_bits << (32-bits-delta_bits));
                bits = sym[m].Bits;
            }
            if( code == (sym[m].Code << (32-sym[m].Bits)) )
            {
                *buf ++ = (unsigned char) sym[m].Symbol;
                break;
            }
        }
    }
}
