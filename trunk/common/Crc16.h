#ifndef _CRC16_H_
#define _CRC16_H_
//
// crc16.h
//
// (C) Copyright 1999-2000 Jan van den Baard.
//     All Rights Reserved.
//

//#include "../standard.h"
#define NULL 0

// Class to perform 16 bit CRC checksumming.
class ClsCrc16
{
public:
	// Constructor/Destructor.
	ClsCrc16();
	ClsCrc16( unsigned short wPolynomial );
	virtual ~ClsCrc16();

	// Implementation
	void CrcInitialize( unsigned short wPolynomial = 0x1021 );
	unsigned short CrcAdd( const unsigned char* pData, unsigned long dwLength );
	unsigned short Crc( unsigned char* pData, unsigned long dwLength );
	inline void CrcReset() { m_sCRC = 0xFFFF; }
	inline unsigned short CrcGet() const { return ( unsigned short )( m_sCRC ^ 0xFFFF ); }
	inline unsigned short CrcTableIndex( int nIndex ) const { return m_pCrcTable[ nIndex ]; };

	// Operator overloads.
	inline bool operator==( unsigned short wCrc ) const { return ( bool )(( m_sCRC ^ 0xFFFF ) == wCrc ); }

protected:
	// Data
	static unsigned short	s_wCrc16[ 256 ];
	unsigned short	       *m_pCrcAllocated;
	unsigned short	       *m_pCrcTable;
	unsigned short		m_sCRC;
};
#endif
