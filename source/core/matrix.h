//==============================================================
//  Graphics Programming 1 (2026-2027)
//  Authors : Matthieu Delaere
//  Copyright (c) 2026 Matthieu Delaere. All rights reserved.
//==============================================================
#ifndef MATRIX_HEADER
#define MATRIX_HEADER

//--- Framework Includes ---
#include <vector3.h>
#include <vector4.h>

//--- Standard Includes ---
#include <atomic>
#include <memory>
#include <mutex>

namespace gfx
{
	//--- Row-Major Matrix ---
	struct Ray;

	struct Matrix final
	{
		//--- Constructors & Destructors ---
		Matrix
		(
			const Vector3& x_axis,
			const Vector3& y_axis,
			const Vector3& z_axis,
			const Vector3& t
		);

		Matrix
		(
			const Vector4& x_axis,
			const Vector4& y_axis,
			const Vector4& z_axis,
			const Vector4& t
		);

		Matrix() = default;

		~Matrix() = default;

		Matrix(const Matrix&);

		Matrix& operator=(const Matrix&);

		Matrix(Matrix&&) noexcept;

		Matrix& operator=(Matrix&&) noexcept;

		//--- Functions ---
		[[nodiscard]] Vector3 TransformVector(const Vector3& v) const;

		[[nodiscard]] Vector3 TransformVector(float x, float y, float z) const;

		[[nodiscard]] Vector4 TransformVector(const Vector4& v) const;

		[[nodiscard]] Vector4 TransformVector
		(float x, float y, float z, float w) const;

		[[nodiscard]] Vector3 TransformPoint(const Vector3& p) const;

		[[nodiscard]] Vector3 TransformPoint(float x, float y, float z) const;

		[[nodiscard]] Vector4 TransformPoint(const Vector4& p) const;

		[[nodiscard]] Vector4 TransformPoint
		(float x, float y, float z, float w) const;

		[[nodiscard]] Vector3 TransformNormal
		(const Vector3& n, bool is_non_uniform = false) const;

		[[nodiscard]] Ray TransformRay(const Ray& ray) const;

		const Matrix& Transpose();

		const Matrix& GetInverse() const;

		[[nodiscard]] float Determinant() const;

		[[nodiscard]] Vector3 GetAxisX() const;

		[[nodiscard]] Vector3 GetAxisY() const;

		[[nodiscard]] Vector3 GetAxisZ() const;

		[[nodiscard]] Vector3 GetTranslation() const;

		[[nodiscard]] Vector3 GetScale() const;

		void Decompose
		(Vector3& scale, Vector3& rotation, Vector3& translation) const;

		static Matrix CreateIdentity();

		static Matrix CreateTranslation(float x, float y, float z);

		static Matrix CreateTranslation(const Vector3& t);

		static Matrix CreateRotationX(float pitch, bool in_degrees = false);

		static Matrix CreateRotationY(float yaw, bool in_degrees = false);

		static Matrix CreateRotationZ(float roll, bool in_degrees = false);

		static Matrix CreateRotation
		(float pitch, float yaw, float roll, bool in_degrees = false);

		static Matrix CreateRotation(const Vector3& r, bool in_degrees = false);

		static Matrix CreateRotationAxis
		(const Vector3& axis, float angle, bool in_degrees = false);

		static Matrix CreateScale(float sx, float sy, float sz);

		static Matrix CreateScale(const Vector3& s);

		static Matrix Transpose(const Matrix& m);

		static Matrix Inverse(const Matrix& m);

		static Matrix CreateLookAtLh
		(
			const Vector3& origin,
			const Vector3& forward,
			const Vector3& up,
			const Vector3& right
		);

		static Matrix CreatePerspectiveFovLH
		(float fov_y, float aspect, float zn, float zf);

		//--- Operators ---
		Vector4& operator[](uint8_t index);

		const Vector4& operator[](uint8_t index) const;

		Matrix operator*(const Matrix& m) const;

		const Matrix& operator*=(const Matrix& m);

		bool operator==(const Matrix& m) const;

		bool operator!=(const Matrix& m) const;

	private:
		Vector4 data[4] {
			{1, 0, 0, 0},
			//xAxis
			{0, 1, 0, 0},
			//yAxis
			{0, 0, 1, 0},
			//zAxis
			{0, 0, 0, 1} //T
		};

		mutable std::atomic<bool> inverse_computed {false};
		mutable std::unique_ptr<Matrix> cached_inverse;
		mutable std::mutex inverse_mutex;

		const Matrix& Inverse();

		void InvalidateCache() const;

		static float ToRadians(float degrees);
	};
}
#endif //MATRIX_HEADER
