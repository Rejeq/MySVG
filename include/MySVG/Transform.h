#pragma once

#include <cmath>

namespace Svg
{
	class Matrix
	{
	public:
		union 
		{
			double m[6];
			struct {
				double m00;
				double m01;
				double m10;
				double m11;
				double m20;
				double m21;
			};
		};

	public:

		Matrix(){ Reset(); }

		inline void Reset()
		{
			m00 = 1; m10 = 0; m20 = 0;
			m01 = 0; m11 = 1; m21 = 0;
		}

		inline void Reset(const double a, const double b, const double c, const double d, const double e, const double f)
		{
			m00 = a; m10 = c; m20 = e;
			m01 = b; m11 = d; m21 = f;
		}


		inline void Transform(const double a, const double b, const double c, const double d, const double e, const double f)
		{
			Reset(a * m00 + b * m10,
				a * m01 + b * m11,
				c * m00 + d * m10,
				c * m01 + d * m11,
				e * m00 + f * m10 + m20,
				e * m01 + f * m11 + m21);
		}

		inline void Transform(const Matrix& mat)
		{
			Transform(mat.m00, mat.m01, mat.m10, mat.m11, mat.m20, mat.m21);
		}

		inline void Transform(const Matrix* mat)
		{
			if(mat != nullptr)
			Transform(mat->m00, mat->m01, mat->m10, mat->m11, mat->m20, mat->m21);
		}

		inline void TranslateX(const double x)
		{
			m20 += x * m00;
			m21 += x * m01;
		}

		inline void TranslateY(const double y)
		{
			m20 += y * m10;
			m21 += y * m11;
		}

		inline void Translate(const double x, const double y)
		{
			m20 += x * m00 + y * m10;
			m21 += x * m01 + y * m11;
		}

		inline void ScaleX(const double sx)
		{
			m00 *= sx; m01 *= sx;
		}

		inline void ScaleY(const double sy)
		{
			m10 *= sy; m11 *= sy;
		}

		inline void Scale(const double sx, const double sy)
		{
			ScaleX(sx);
			ScaleY(sy);
		}

		void Skew(const double x, const double y)
		{
			const double tanX = std::tan(ToRadians(x));
			const double tanY = std::tan(ToRadians(y));

			double b00 = tanY * m10;
			double b01 = tanY * m11;

			m10 += tanX * m00;
			m11 += tanX * m01;

			m00 += b00;
			m01 += b01;
		}

		void Rotate(const double angle)
		{
			double sn = std::sin(ToRadians(angle));
			double cs = std::cos(ToRadians(angle));

			double b00 = sn * m10 + cs * m00;
			double b01 = sn * m11 + cs * m01;
			double b10 = cs * m10 - sn * m00;
			double b11 = cs * m11 - sn * m01;

			m00 = b00; m01 = b01;
			m10 = b10; m11 = b11;
		}

		void Rotate(const double angle, const double cx, const double cy)
		{
			double sin = std::sin(ToRadians(angle));
			double cos = std::cos(ToRadians(angle));

			double b00 = sin * m10 + cos * m00;
			double b01 = sin * m11 + cos * m01;
			double b10 = cos * m10 - sin * m00;
			double b11 = cos * m11 - sin * m01;

			double bx = cx - cos * cx + sin * cy;
			double by = cy - sin * cx - cos * cy;

			m20 = bx * m00 + by * m10 + m20;
			m21 = bx * m01 + by * m11 + m21;

			m00 = b00; m01 = b01;
			m10 = b10; m11 = b11;
		}


		void PostTransform(const double a, const double b, const double c, const double d, const double e, const double f)
		{
			Reset(	m00 * a + m01 * c,
					m00 * b + m01 * d,
					m10 * a + m11 * c,
					m10 * b + m11 * d,
					m20 * a + m21 * c + e,
					m20 * b + m21 * d + f);
		}

		void PostTransform(const Matrix& mat)
		{
			PostTransform(mat.m00, mat.m01, mat.m10, mat.m11, mat.m20, mat.m21);
		}

		inline void PostTransform(const Matrix* mat)
		{
			if (mat != nullptr)
				PostTransform(mat->m00, mat->m01, mat->m10, mat->m11, mat->m20, mat->m21);
		}
		
		inline void PostTranslateX(const double x) { m20 += x; }
		inline void PostTranslateY(const double y) { m21 += y; }
		inline void PostTranslate(const double x, const double y)
		{
			PostTranslateX(x);
			PostTranslateY(y);
		}

		inline void PostScaleX(const double sx)
		{
			m00 *= sx;
			m10 *= sx;
			m20 *= sx;
		}

		inline void PostScaleY(const double sy)
		{
			m01 *= sy;
			m11 *= sy;
			m21 *= sy;
		}

		inline void PostScale(const double sx, const double sy)
		{
			PostScaleX(sx);
			PostScaleY(sy);
		}

		inline void PostSkewX(const double x)
		{
			const double tn = std::tan(ToRadians(x));

			m00 += m01 * tn;
			m10 += m11 * tn;
			m20 += m21 * tn;
		}

		inline void PostSkewY(const double y)
		{
			const double tn = std::tan(ToRadians(y));

			m01 += m00 * tn;
			m11 += m10 * tn;
			m21 += m20 * tn;
		}
		
		void PostSkew(const double x, const double y)
		{
			const double xtn = std::tan(ToRadians(x));
			const double ytn = std::tan(ToRadians(y));

			const double b00 = m01 * xtn;
			const double b10 = m11 * xtn;
			const double b20 = m21 * xtn;

			m01 += m00 * ytn;
			m11 += m10 * ytn;
			m21 += m20 * ytn;

			m00 += b00;
			m10 += b10;
			m20 += b20;
		}

		void PostRotate(const double angle)
		{
			double sn = std::sin(ToRadians(angle));
			double cs = std::cos(ToRadians(angle));

			double b00 = m00 * cs - m01 * sn;
			double b01 = m00 * sn + m01 * cs;
			double b10 = m10 * cs - m11 * sn;
			double b11 = m10 * sn + m11 * cs;
			double b20 = m20 * cs - m21 * sn;
			double b21 = m20 * sn + m21 * cs;

			Reset(b00, b01, b10, b11, b20, b21);
		}

		void PostRotate(const double angle, const double cx, const double cy)
		{
			double sn = std::sin(ToRadians(angle));
			double cs = std::cos(ToRadians(angle));

			double b00 = m00 * cs - m01 * sn;
			double b01 = m00 * sn + m01 * cs;
			double b10 = m10 * cs - m11 * sn;
			double b11 = m10 * sn + m11 * cs;
			double b20 = (m20 * cs - m21 * sn) + cx - cs * cx + sn * cy;
			double b21 = (m20 * sn + m21 * cs) + cy - sn * cx - cs * cy;

			Reset(b00, b01, b10, b11, b20, b21);
		}

		void Overlay(Matrix* mat)
		{
			static const Matrix defaultVal;
			if (mat == nullptr)
				return;
			
			if (memcmp(m, defaultVal.m, sizeof(m)) == 0)
				memcpy(m, mat->m, sizeof(m));
		}

		Matrix operator*(const Matrix& right)
		{
			PostTransform(right);
			return *this;
		}

		Matrix& operator*=(const Matrix& right)
		{
			PostTransform(right);
			return *this;
		}
	};
}