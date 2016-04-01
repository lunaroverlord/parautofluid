/*
 * Camera object, contains purely the math of moving around
 */
#pragma once 
#include <cmath>

struct Vector4
{
	float x, y, z, w;

	Vector4() : Vector4(0, 0, 0, 0) {}

	Vector4(float x0, float y0, float z0, float w0 = 0)
		: x(x0), y(y0), z(z0), w(w0) {}


	float* float4()
	{
		return (float*)this;
	}

	double length() const
        {
                return sqrt(x*x + y*y + z*z);
        }

	Vector4 normalize()
        {
                double l = length();
                if(l != 0)
                        {x /= l; y /= l; z /= l;}
		return *this;
        }
	Vector4 cross(const Vector4& arg)
	{
		return Vector4(
		(y * arg.z) - (z * arg.y),
		(z * arg.x) - (x * arg.z),
		(x * arg.y) - (y * arg.x) );
	}

	Vector4 operator*(const float i) const
        {
                return Vector4(x * i, y * i, z * i);
        }

	void operator+=(const Vector4& v)
        {
                x += v.x;
                y += v.y;
		z += v.z;
        }

        void operator-=(const Vector4& v)
        {
                x -= v.x;
                y -= v.y;
		z -= v.z;
        }

	Vector4 operator-()
	{
		return Vector4(-x, -y, -z);
	}
};

class Camera
{
public:
	Camera()
	{
		//Defaults
		position = Vector4(0.5, 0.5, -5, 0);
		forward = Vector4(0, 0, 1, 0).normalize();
		up = Vector4(0, 1, 0, 0).normalize();
		right = -forward.cross(up);

		speed = 0.25;
		lookSpeed = 0.02;
	}

	//Getters (for arrays)
	float** getPosition() {return (float**)&position;}
	float** getForward() {return (float**)&forward;}
	float** getUp() {return (float**)&up;}
	float** getRight() {return (float**)&right;}

	//If a key is held down, this will keep moving the camera
	void update()
	{
		move(held);
	}

	//Toggle one movement
	enum Movement {STILL, LEFT, RIGHT, UP, DOWN, FORWARD, BACKWARD, LOOK_LEFT, LOOK_RIGHT, LOOK_UP, LOOK_DOWN};
	void hold(Movement m)
	{
		held = m;
	}
	void move(Movement m)
	{
		switch(m)
		{
			case LEFT:
				position -= right * speed;
			break;
			case RIGHT:
				position += right * speed;
			break;
			case UP:
				position += up * speed;
			break;
			case DOWN:
				position -= up * speed;
			break;
			case FORWARD:
				position += forward * speed;
			break;
			case BACKWARD:
				position -= forward * speed;
			break;

			case LOOK_LEFT:
				forward -= right * lookSpeed;
				right = -forward.cross(up);
			break;
			case LOOK_RIGHT:
				forward += right * lookSpeed;
				right = -forward.cross(up);
			break;
			case LOOK_UP:
				forward += up * lookSpeed;
				up = forward.cross(right);
			break;
			case LOOK_DOWN:
				forward -= up * lookSpeed;
				up = forward.cross(right);
			break;
		}
		if(m >= LOOK_LEFT)
		{
			forward.normalize();
			right.normalize();
			up.normalize();
		}
	}

private:
	Vector4 position;
	Vector4 forward;
	Vector4 up;
	Vector4 right;

	Movement held;
	float speed, lookSpeed;

};
