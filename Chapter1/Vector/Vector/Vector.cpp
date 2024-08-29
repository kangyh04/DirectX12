// Vector.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <iostream>

using namespace std;
using namespace DirectX;
using namespace DirectX::PackedVector;

ostream& XM_CALLCONV operator<<(ostream& os, FXMVECTOR v)
{
    XMFLOAT3 dest;
    XMStoreFloat3(&dest, v);

    os << "(" << dest.x << ", " << dest.y << ", " << dest.z << ")";
    return os;
}

int main()
{
    cout.setf(ios_base::boolalpha);

    if (!XMVerifyCPUSupport())
    {
        cout << "does not support DirectXMath" << endl;
        return 0;
    }

//     XMVECTOR p = XMVectorZero();
//     XMVECTOR q = XMVectorSplatOne();
//     XMVECTOR u = XMVectorSet(1.0f, 2.0f, 3.0f, 0.0f);
//     XMVECTOR v = XMVectorReplicate(-2.0f);
//     XMVECTOR w = XMVectorSplatZ(u);
// 
//     cout << "p = " << p << endl;
//     cout << "q = " << q << endl;
//     cout << "u = " << u << endl;
//     cout << "v = " << v << endl;
//     cout << "w = " << w << endl;

//     XMVECTOR n = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
//     XMVECTOR u = XMVectorSet(1.0f, 2.0f, 3.0f, 0.0f);
//     XMVECTOR v = XMVectorSet(-2.0f, 1.0f, -3.0f, 0.0f);
//     XMVECTOR w = XMVectorSet(0.707f, 0.707f, 0.0f, 0.0f);
// 
//     XMVECTOR a = u + v;
// 
//     XMVECTOR b = u - v;
// 
//     XMVECTOR c = 10.0f * u;
// 
//     XMVECTOR L = XMVector3Length(u);
// 
//     XMVECTOR d = XMVector3Normalize(u);
// 
//     XMVECTOR s = XMVector3Dot(u, v);
// 
//     XMVECTOR e = XMVector3Cross(u, v);
// 
//     XMVECTOR projW;
//     XMVECTOR perpW;
// 
//     XMVector3ComponentsFromNormal(&projW, &perpW, w, n);
// 
//     bool equal = XMVector3Equal(projW + perpW, w) != 0;
//     bool notEqual = XMVector3NotEqual(projW + perpW, w) != 0;
// 
//     XMVECTOR angleVec = XMVector3AngleBetweenVectors(projW, perpW);
//     float angleRadians = XMVectorGetX(angleVec);
//     float angleDegrees = XMConvertToDegrees(angleRadians);
// 
//     cout << "u                  = " << u << endl;
//     cout << "v                  = " << v << endl;
//     cout << "w                  = " << w << endl;
//     cout << "n                  = " << n << endl;
//     cout << "u + v              = " << a << endl;
//     cout << "u - v              = " << b << endl;
//     cout << "10 * u             = " << c << endl;
//     cout << "normal             = " << d << endl;
//     cout << "cross              = " << e << endl;
//     cout << "length             = " << L << endl;
//     cout << "dot                = " << s << endl;
//     cout << "projW              = " << projW << endl;
//     cout << "perpW              = " << perpW << endl;
//     cout << "projW + perpW == w = " << equal << endl;
//     cout << "projW + perpW != w = " << notEqual << endl;
//     cout << "angle              = " << angleDegrees << endl;

    return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
