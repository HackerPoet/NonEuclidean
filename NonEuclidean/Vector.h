#pragma once
#include <cmath>
#include <cfloat>
#include <memory>
#include <iostream>
#include <cassert>

class Vector3 {
public:
  //Constructors
  Vector3() {}
  explicit Vector3(float b) : x(b), y(b), z(b) {}
  explicit Vector3(const float* b) : x(b[0]), y(b[1]), z(b[2]) {}
  explicit Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

  //General
  inline static Vector3 Zero() { return Vector3(0.0f); }
  inline static Vector3 Ones() { return Vector3(1.0f); }
  inline static Vector3 UnitX() { return Vector3(1, 0, 0); }
  inline static Vector3 UnitY() { return Vector3(0, 1, 0); }
  inline static Vector3 UnitZ() { return Vector3(0, 0, 1); }

  //Setters
  inline void Set(float _x, float _y, float _z) { x = _x; y = _y; z = _z; }
  inline void SetZero() { x = 0.0f; y = 0.0f; z = 0.0f; }
  inline void SetOnes() { x = 1.0f; y = 1.0f; z = 1.0f; }
  inline void SetUnitX() { x = 1.0f; y = 0.0f; z = 0.0f; }
  inline void SetUnitY() { x = 0.0f; y = 1.0f; z = 0.0f; }
  inline void SetUnitZ() { x = 0.0f; y = 0.0f; z = 1.0f; }

  //Basic operatios
  inline Vector3 operator+(float b) const {
    return Vector3(x + b, y + b, z + b);
  }
  inline Vector3 operator-(float b) const {
    return Vector3(x - b, y - b, z - b);
  }
  inline Vector3 operator*(float b) const {
    return Vector3(x * b, y * b, z * b);
  }
  inline Vector3 operator/(float b) const {
    return Vector3(x / b, y / b, z / b);
  }
  inline Vector3 operator+(const Vector3& b) const {
    return Vector3(x + b.x, y + b.y, z + b.z);
  }
  inline Vector3 operator-(const Vector3& b) const {
    return Vector3(x - b.x, y - b.y, z - b.z);
  }
  inline Vector3 operator*(const Vector3& b) const {
    return Vector3(x * b.x, y * b.y, z * b.z);
  }
  inline Vector3 operator/(const Vector3& b) const {
    return Vector3(x / b.x, y / b.y, z / b.z);
  }
  inline void operator+=(float b) {
    x += b; y += b; z += b;
  }
  inline void operator-=(float b) {
    x -= b; y -= b; z -= b;
  }
  inline void operator*=(float b) {
    x *= b; y *= b; z *= b;
  }
  inline void operator/=(float b) {
    x /= b; y /= b; z /= b;
  }
  inline void operator+=(const Vector3& b) {
    x += b.x; y += b.y; z += b.z;
  }
  inline void operator-=(const Vector3& b) {
    x -= b.x; y -= b.y; z -= b.z;
  }
  inline void operator*=(const Vector3& b) {
    x *= b.x; y *= b.y; z *= b.z;
  }
  inline void operator/=(const Vector3& b) {
    x /= b.x; y /= b.y; z /= b.z;
  }
  inline Vector3 operator-() const {
    return Vector3(-x, -y, -z);
  }

  //Vector algebra
  inline float Dot(const Vector3& b) const {
    return x*b.x + y*b.y + z*b.z;
  }
  inline Vector3 Cross(const Vector3& b) const {
    return Vector3(y*b.z - z*b.y, z*b.x - x*b.z, x*b.y - y*b.x);
  }
  inline float MagSq() const {
    return x*x + y*y + z*z;
  }
  inline float Mag() const {
    return std::sqrt(MagSq());
  }

  //Normalization
  inline void Normalize() {
    (*this) /= Mag();
  }
  inline void NormalizeSafe() {
    (*this) /= (Mag() + FLT_EPSILON);
  }
  inline Vector3 Normalized() const {
    return (*this) / Mag();
  }
  inline Vector3 NormalizedSafe() const {
    return (*this) / (Mag() + FLT_EPSILON);
  }
  inline float Angle(const Vector3& b) const {
    return std::acos(Normalized().Dot(b.Normalized()));
  }
  inline float AngleSafe(const Vector3& b) const {
    return std::acos(NormalizedSafe().Dot(b.NormalizedSafe()));
  }
  inline void ClipMag(float m) {
    assert(m > 0.0f);
    const float r = MagSq() / (m * m);
    if (r > 1.0f) {
      (*this) /= std::sqrt(r);
    }
  }

  //Other
  inline bool IsNDC() const {
    return (x > -1.0f && x < 1.0f && y > -1.0f && y < 1.0f && z > -1.0f && z < 1.0f);
  }

  //Components
  float x, y, z;
};
inline Vector3 operator/(float b, const Vector3& v) {
  return Vector3(b / v.x, b / v.y, b / v.z);
}
inline void operator/=(float b, Vector3& v) {
  v.x = b / v.x; v.y = b / v.y; v.z = b / v.z;
}

class Vector4 {
public:
  Vector4() {}
  explicit Vector4(float b) : x(b), y(b), z(b), w(b) {}
  explicit Vector4(const Vector3& xyz, float w) : x(xyz.x), y(xyz.y), z(xyz.z), w(w) {}
  explicit Vector4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

  inline Vector3 XYZ() const { return Vector3(x, y, z); }
  inline Vector3 XYZNormalized() const { return Vector3(x, y, z).Normalized(); }
  inline Vector3 Homogenized() const { return Vector3(x/w, y/w, z/w); }

  inline Vector4 operator*(float b) const {
    return Vector4(x * b, y * b, z * b, w * b);
  }
  inline Vector4 operator/(float b) const {
    return Vector4(x / b, y / b, z / b, w / b);
  }
  inline void operator*=(float b) {
    x *= b; y *= b; z *= b; w *= b;
  }
  inline void operator/=(float b) {
    x /= b; y /= b; z /= b; w /= b;
  }

  inline float Dot(const Vector4& b) const { return x*b.x + y*b.y + z*b.z + w*b.w; }

  //Components
  float x, y, z, w;
};

class Matrix4 {
public:
  //Constructors
  Matrix4() {}
  explicit Matrix4(float b) { Fill(b); }

  //General
  inline void Fill(float b) {
    std::fill(m, m + 16, b);
  }
  inline void MakeZero() {
    Fill(0.0f);
  }
  inline void MakeIdentity() {
    m[0]  = 1.0f; m[1]  = 0.0f; m[2]  = 0.0f; m[3]  = 0.0f;
    m[4]  = 0.0f; m[5]  = 1.0f; m[6]  = 0.0f; m[7]  = 0.0f;
    m[8]  = 0.0f; m[9]  = 0.0f; m[10] = 1.0f; m[11] = 0.0f;
    m[12] = 0.0f; m[13] = 0.0f; m[14] = 0.0f; m[15] = 1.0f;
  }
  inline void MakeRotX(float a) {
    m[0] =  1.0f; m[1]  = 0.0f;        m[2]  = 0.0f;         m[3]  = 0.0f;
    m[4] =  0.0f; m[5]  = std::cos(a); m[6]  = -std::sin(a); m[7]  = 0.0f;
    m[8] =  0.0f; m[9]  = std::sin(a); m[10] = std::cos(a);  m[11] = 0.0f;
    m[12] = 0.0f; m[13] = 0.0f;        m[14] = 0.0f;         m[15] = 1.0f;
  }
  inline void MakeRotY(float a) {
    m[0]  = std::cos(a);  m[1]  = 0.0f; m[2]  = std::sin(a); m[3]  = 0.0f;
    m[4]  = 0.0f;         m[5]  = 1.0f; m[6]  = 0.0f;        m[7]  = 0.0f;
    m[8]  = -std::sin(a); m[9]  = 0.0f; m[10] = std::cos(a); m[11] = 0.0f;
    m[12] = 0.0f;         m[13] = 0.0f; m[14] = 0.0f;        m[15] = 1.0f;
  }
  inline void MakeRotZ(float a) {
    m[0]  = std::cos(a); m[1]  = -std::sin(a); m[2]  = 0.0f; m[3]  = 0.0f;
    m[4]  = std::sin(a); m[5]  = std::cos(a);  m[6]  = 0.0f; m[7]  = 0.0f;
    m[8]  = 0.0f;        m[9]  = 0.0f;         m[10] = 1.0f; m[11] = 0.0f;
    m[12] = 0.0f;        m[13] = 0.0f;         m[14] = 0.0f; m[15] = 1.0f;
  }
  inline void MakeTrans(const Vector3& t) {
    m[0]  = 1.0f; m[1]  = 0.0f; m[2]  = 0.0f; m[3]  = t.x;
    m[4]  = 0.0f; m[5]  = 1.0f; m[6]  = 0.0f; m[7]  = t.y;
    m[8]  = 0.0f; m[9]  = 0.0f; m[10] = 1.0f; m[11] = t.z;
    m[12] = 0.0f; m[13] = 0.0f; m[14] = 0.0f; m[15] = 1.0f;
  }
  inline void MakeScale(const Vector3& s) {
    m[0]  = s.x;  m[1]  = 0.0f; m[2]  = 0.0f; m[3]  = 0.0f;
    m[4]  = 0.0f; m[5]  = s.y;  m[6]  = 0.0f; m[7]  = 0.0f;
    m[8]  = 0.0f; m[9]  = 0.0f; m[10] = s.z;  m[11] = 0.0f;
    m[12] = 0.0f; m[13] = 0.0f; m[14] = 0.0f; m[15] = 1.0f;
  }

  //Statics
  inline static Matrix4 Zero() { Matrix4 m; m.MakeZero(); return m; }
  inline static Matrix4 Identity() { Matrix4 m; m.MakeIdentity(); return m; }
  inline static Matrix4 RotX(float a) { Matrix4 m; m.MakeRotX(a); return m; }
  inline static Matrix4 RotY(float a) { Matrix4 m; m.MakeRotY(a); return m; }
  inline static Matrix4 RotZ(float a) { Matrix4 m; m.MakeRotZ(a); return m; }
  inline static Matrix4 Trans(const Vector3& t) { Matrix4 m; m.MakeTrans(t); return m; }
  inline static Matrix4 Scale(float s) { Matrix4 m; m.MakeScale(Vector3(s)); return m; }
  inline static Matrix4 Scale(const Vector3& s) { Matrix4 m; m.MakeScale(s); return m; }

  //Some getters
  inline Vector3 XAxis() const {
    return Vector3(m[0], m[4], m[8]);
  }
  inline Vector3 YAxis() const {
    return Vector3(m[1], m[5], m[9]);
  }
  inline Vector3 ZAxis() const {
    return Vector3(m[2], m[6], m[10]);
  }
  inline Vector3 Translation() const {
    return Vector3(m[3], m[7], m[11]);
  }
  inline Vector3 Scale() const {
    return Vector3(m[0], m[5], m[10]);
  }

  //Setters
  inline void SetTranslation(const Vector3& t) {
    m[3] = t.x;
    m[7] = t.y;
    m[11] = t.z;
  }
  inline void SetXAxis(const Vector3& t) {
    m[0] = t.x;
    m[4] = t.y;
    m[8] = t.z;
  }
  inline void SetYAxis(const Vector3& t) {
    m[1] = t.x;
    m[5] = t.y;
    m[9] = t.z;
  }
  inline void SetZAxis(const Vector3& t) {
    m[2] = t.x;
    m[6] = t.y;
    m[10] = t.z;
  }
  inline void SetScale(const Vector3& s) {
    m[0] = s.x;
    m[5] = s.y;
    m[10] = s.z;
  }

  //Transformations
  inline Matrix4 Transposed() const {
    Matrix4 out;
    out.m[0]  = m[0]; out.m[1]  = m[4]; out.m[2]  = m[8];  out.m[3]  = m[12];
    out.m[4]  = m[1]; out.m[5]  = m[5]; out.m[6]  = m[9];  out.m[7]  = m[13];
    out.m[8]  = m[2]; out.m[9]  = m[6]; out.m[10] = m[10]; out.m[11] = m[14];
    out.m[12] = m[3]; out.m[13] = m[7]; out.m[14] = m[11]; out.m[15] = m[15];
    return out;
  }
  inline void Translate(const Vector3& t) {
    m[3] += t.x;
    m[7] += t.y;
    m[11] += t.z;
  }
  inline void Stretch(const Vector3& s) {
    m[0] *= s.x;
    m[5] *= s.y;
    m[10] *= s.z;
  }

  //Basic operatios
  inline Matrix4 operator+(const Matrix4& b) const {
    Matrix4 out;
    for (int i = 0; i < 16; ++i) { out.m[i] = m[i] + b.m[i]; }
    return out;
  }
  inline Matrix4 operator-(const Matrix4& b) const {
    Matrix4 out;
    for (int i = 0; i < 16; ++i) { out.m[i] = m[i] - b.m[i]; }
    return out;
  }
  inline void operator+=(const Matrix4& b) {
    for (int i = 0; i < 16; ++i) { m[i] += b.m[i]; }
  }
  inline void operator-=(const Matrix4& b) {
    for (int i = 0; i < 16; ++i) { m[i] -= b.m[i]; }
  }
  inline void operator*=(float b) {
    for (int i = 0; i < 16; ++i) { m[i] *= b; }
  }
  inline void operator/=(float b) {
    operator*=(1.0f / b);
  }

  //Multiplication
  Matrix4 operator*(const Matrix4& b) const {
    Matrix4 out;
    out.m[0]  = b.m[0]*m[0]  + b.m[4]*m[1]  + b.m[8] *m[2]  + b.m[12]*m[3];
    out.m[1]  = b.m[1]*m[0]  + b.m[5]*m[1]  + b.m[9] *m[2]  + b.m[13]*m[3];
    out.m[2]  = b.m[2]*m[0]  + b.m[6]*m[1]  + b.m[10]*m[2]  + b.m[14]*m[3];
    out.m[3]  = b.m[3]*m[0]  + b.m[7]*m[1]  + b.m[11]*m[2]  + b.m[15]*m[3];

    out.m[4]  = b.m[0]*m[4]  + b.m[4]*m[5]  + b.m[8] *m[6]  + b.m[12]*m[7];
    out.m[5]  = b.m[1]*m[4]  + b.m[5]*m[5]  + b.m[9] *m[6]  + b.m[13]*m[7];
    out.m[6]  = b.m[2]*m[4]  + b.m[6]*m[5]  + b.m[10]*m[6]  + b.m[14]*m[7];
    out.m[7]  = b.m[3]*m[4]  + b.m[7]*m[5]  + b.m[11]*m[6]  + b.m[15]*m[7];

    out.m[8]  = b.m[0]*m[8]  + b.m[4]*m[9]  + b.m[8] *m[10] + b.m[12]*m[11];
    out.m[9]  = b.m[1]*m[8]  + b.m[5]*m[9]  + b.m[9] *m[10] + b.m[13]*m[11];
    out.m[10] = b.m[2]*m[8]  + b.m[6]*m[9]  + b.m[10]*m[10] + b.m[14]*m[11];
    out.m[11] = b.m[3]*m[8]  + b.m[7]*m[9]  + b.m[11]*m[10] + b.m[15]*m[11];

    out.m[12] = b.m[0]*m[12] + b.m[4]*m[13] + b.m[8] *m[14] + b.m[12]*m[15];
    out.m[13] = b.m[1]*m[12] + b.m[5]*m[13] + b.m[9] *m[14] + b.m[13]*m[15];
    out.m[14] = b.m[2]*m[12] + b.m[6]*m[13] + b.m[10]*m[14] + b.m[14]*m[15];
    out.m[15] = b.m[3]*m[12] + b.m[7]*m[13] + b.m[11]*m[14] + b.m[15]*m[15];
    return out;
  }
  void operator*=(const Matrix4& b) {
    (*this) = operator*(b);
  }
  Vector4 operator*(const Vector4& b) const {
    return Vector4(
      m[0]*b.x  + m[1]*b.y  + m[2]*b.z  + m[3]*b.w,
      m[4]*b.x  + m[5]*b.y  + m[6]*b.z  + m[7]*b.w,
      m[8]*b.x  + m[9]*b.y  + m[10]*b.z + m[11]*b.w,
      m[12]*b.x + m[13]*b.y + m[14]*b.z + m[15]*b.w
    );
  }
  Vector3 MulPoint(const Vector3& b) const {
    const Vector3 p(
      m[0] * b.x + m[1] * b.y + m[2] * b.z + m[3],
      m[4] * b.x + m[5] * b.y + m[6] * b.z + m[7],
      m[8] * b.x + m[9] * b.y + m[10] * b.z + m[11]
    );
    const float w = m[12] * b.x + m[13] * b.y + m[14] * b.z + m[15];
    return p / w;
  }
  Vector3 MulDirection(const Vector3& b) const {
    return Vector3(
      m[0] * b.x + m[1] * b.y + m[2] * b.z,
      m[4] * b.x + m[5] * b.y + m[6] * b.z,
      m[8] * b.x + m[9] * b.y + m[10] * b.z
    );
  }

  //Inverse
  Matrix4 Inverse() const {
    Matrix4 inv;

    inv.m[0] = m[5] * m[10] * m[15] -
      m[5] * m[11] * m[14] -
      m[9] * m[6] * m[15] +
      m[9] * m[7] * m[14] +
      m[13] * m[6] * m[11] -
      m[13] * m[7] * m[10];

    inv.m[4] = -m[4] * m[10] * m[15] +
      m[4] * m[11] * m[14] +
      m[8] * m[6] * m[15] -
      m[8] * m[7] * m[14] -
      m[12] * m[6] * m[11] +
      m[12] * m[7] * m[10];

    inv.m[8] = m[4] * m[9] * m[15] -
      m[4] * m[11] * m[13] -
      m[8] * m[5] * m[15] +
      m[8] * m[7] * m[13] +
      m[12] * m[5] * m[11] -
      m[12] * m[7] * m[9];

    inv.m[12] = -m[4] * m[9] * m[14] +
      m[4] * m[10] * m[13] +
      m[8] * m[5] * m[14] -
      m[8] * m[6] * m[13] -
      m[12] * m[5] * m[10] +
      m[12] * m[6] * m[9];

    inv.m[1] = -m[1] * m[10] * m[15] +
      m[1] * m[11] * m[14] +
      m[9] * m[2] * m[15] -
      m[9] * m[3] * m[14] -
      m[13] * m[2] * m[11] +
      m[13] * m[3] * m[10];

    inv.m[5] = m[0] * m[10] * m[15] -
      m[0] * m[11] * m[14] -
      m[8] * m[2] * m[15] +
      m[8] * m[3] * m[14] +
      m[12] * m[2] * m[11] -
      m[12] * m[3] * m[10];

    inv.m[9] = -m[0] * m[9] * m[15] +
      m[0] * m[11] * m[13] +
      m[8] * m[1] * m[15] -
      m[8] * m[3] * m[13] -
      m[12] * m[1] * m[11] +
      m[12] * m[3] * m[9];

    inv.m[13] = m[0] * m[9] * m[14] -
      m[0] * m[10] * m[13] -
      m[8] * m[1] * m[14] +
      m[8] * m[2] * m[13] +
      m[12] * m[1] * m[10] -
      m[12] * m[2] * m[9];

    inv.m[2] = m[1] * m[6] * m[15] -
      m[1] * m[7] * m[14] -
      m[5] * m[2] * m[15] +
      m[5] * m[3] * m[14] +
      m[13] * m[2] * m[7] -
      m[13] * m[3] * m[6];

    inv.m[6] = -m[0] * m[6] * m[15] +
      m[0] * m[7] * m[14] +
      m[4] * m[2] * m[15] -
      m[4] * m[3] * m[14] -
      m[12] * m[2] * m[7] +
      m[12] * m[3] * m[6];

    inv.m[10] = m[0] * m[5] * m[15] -
      m[0] * m[7] * m[13] -
      m[4] * m[1] * m[15] +
      m[4] * m[3] * m[13] +
      m[12] * m[1] * m[7] -
      m[12] * m[3] * m[5];

    inv.m[14] = -m[0] * m[5] * m[14] +
      m[0] * m[6] * m[13] +
      m[4] * m[1] * m[14] -
      m[4] * m[2] * m[13] -
      m[12] * m[1] * m[6] +
      m[12] * m[2] * m[5];

    inv.m[3] = -m[1] * m[6] * m[11] +
      m[1] * m[7] * m[10] +
      m[5] * m[2] * m[11] -
      m[5] * m[3] * m[10] -
      m[9] * m[2] * m[7] +
      m[9] * m[3] * m[6];

    inv.m[7] = m[0] * m[6] * m[11] -
      m[0] * m[7] * m[10] -
      m[4] * m[2] * m[11] +
      m[4] * m[3] * m[10] +
      m[8] * m[2] * m[7] -
      m[8] * m[3] * m[6];

    inv.m[11] = -m[0] * m[5] * m[11] +
      m[0] * m[7] * m[9] +
      m[4] * m[1] * m[11] -
      m[4] * m[3] * m[9] -
      m[8] * m[1] * m[7] +
      m[8] * m[3] * m[5];

    inv.m[15] = m[0] * m[5] * m[10] -
      m[0] * m[6] * m[9] -
      m[4] * m[1] * m[10] +
      m[4] * m[2] * m[9] +
      m[8] * m[1] * m[6] -
      m[8] * m[2] * m[5];

    const float det = m[0] * inv.m[0] + m[1] * inv.m[4] + m[2] * inv.m[8] + m[3] * inv.m[12];
    inv /= det;
    return inv;
  }

  //Components
  float m[16];
};

//Debug printing
inline std::ostream& operator<<(std::ostream& out, const Vector3& v) {
  out << v.x << ", " << v.y << ", " << v.z;
  return out;
}
inline std::ostream& operator<<(std::ostream& out, const Vector4& v) {
  out << v.x << ", " << v.y << ", " << v.z << ", " << v.w;
  return out;
}
inline std::ostream& operator<<(std::ostream& out, const Matrix4& m) {
  out << m.m[0]  << ", " << m.m[1]  << ", " << m.m[2]  << ", " << m.m[3] << "\n";
  out << m.m[4]  << ", " << m.m[5]  << ", " << m.m[6]  << ", " << m.m[7] << "\n";
  out << m.m[8]  << ", " << m.m[9]  << ", " << m.m[10] << ", " << m.m[11] << "\n";
  out << m.m[12] << ", " << m.m[13] << ", " << m.m[14] << ", " << m.m[15];
  return out;
}
