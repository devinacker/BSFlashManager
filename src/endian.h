#pragma once

#include <QtEndian>

template<typename T, bool big = false>
struct Endian
{
	T m_val;

	Endian<T, big>& operator = (const T& val)
	{
		if (big)
			m_val = qToBigEndian(val);
		else
			m_val = qToLittleEndian(val);

		return *this;
	}

	operator T() const
	{
		if (big)
			return qFromBigEndian(m_val);
		else
			return qFromLittleEndian(m_val);
	}
};

template<bool big>
struct Endian<float, big>
{
	union val
	{
		float  f;
		qint32 i;
	} m_val;

	Endian<float, big>& operator = (const float& val)
	{
		m_val.f = val;

		if (big)
			m_val.i = qToBigEndian(m_val.i);
		else
			m_val.i = qToLittleEndian(m_val.i);

		return *this;
	}

	operator float() const
	{
		val v;

		if (big)
			v.i = qFromBigEndian(m_val.i);
		else
			v.i = qFromLittleEndian(m_val.i);

		return v.f;
	}
};

template<bool big>
struct Endian<double, big>
{
	union val
	{
		double f;
		qint64 i;
	} m_val;

	Endian<double, big>& operator = (const double& val)
	{
		m_val.f = val;

		if (big)
			m_val.i = qToBigEndian(m_val.i);
		else
			m_val.i = qToLittleEndian(m_val.i);

		return *this;
	}

	operator double() const
	{
		val v;

		if (big)
			v.i = qFromBigEndian(m_val.i);
		else
			v.i = qFromLittleEndian(m_val.i);

		return v.f;
	}
};

typedef qint8  int8;
typedef quint8 uint8;

typedef Endian<qint16>  int16le;
typedef Endian<quint16> uint16le;
typedef Endian<qint32>  int32le;
typedef Endian<quint32> uint32le;

typedef Endian<float, false>  float32le;
typedef Endian<double, false> float64le;

typedef Endian<qint16, true>  int16be;
typedef Endian<quint16, true> uint16be;
typedef Endian<qint32, true>  int32be;
typedef Endian<quint32, true> uint32be;

typedef Endian<float, true>   float32be;
typedef Endian<double, true>  float64be;
