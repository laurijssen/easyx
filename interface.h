#ifndef __INTERFACE_H__
#define __INTERFACE_H__

#include <events.h>

class Component
{
public:
	Component();

protected:
	short x;
	short y;
	short width;
	short height;
};

class Window : public Component
{
public:
};

class TextField : public Component
{
public:	
};

class Button : public Component
{
public:
};

class Picture : public Component
{
public:
};

class Label : public Component
{
public:
};

#endif //__INTERFACE_H__