#include <GameElement.h>
#include <string>

EASYDRAW *Entity::DDraw = NULL; // points to the ddrawobject in GameManager

Entity::Entity() : dim(0, 0, 64, 64)
{
	speed = 1, direction = SOUTH;
}

Entity::Entity(U16 x, U16 y, U16 w, U16 h) : dim(x, y, w, h)
{
	speed = 1;
}

Entity::~Entity()
{
}

U16 Entity::GetWidth() { return dim.w; }

U16 Entity::GetHeight() { return dim.h; }

U16 Entity::GetX() { return dim.x; }

U16 Entity::GetY() { return dim.y; }

void Entity::SetSpeed(U8 speed) { this->speed = speed; }

U8 Entity::GetSpeed() { return speed; }

Dimension *Entity::GetDimensions()
{
	return &dim;
}

/***************************************************************
before loading images to this class. the easydraw pointer has to be set
The rest of the class will assume this has happened and perform no checks
***************************************************************/
void Entity::SetEasyDraw(EASYDRAW *DDraw)
{
	if( !Entity::DDraw )
		Entity::DDraw = DDraw;
}

/**********************************************************************/

VisualEntity::VisualEntity() : Entity() {}

VisualEntity::VisualEntity(U16 x, U16 y, U16 w, U16 h, char *image) 
	: Entity(x, y, w, h)
{
	if( image )
		Load(image);
}

VisualEntity::~VisualEntity()
{
	DDraw->ReleaseSurface(image);
}

void VisualEntity::Load(char *image)
{
	this->image = DDraw->CreateImage(dim.w, dim.h, 0, image);
}

void VisualEntity::Draw(U32 param)
{
	DDraw->DrawImage(dim.x, dim.y, image, param);
}

Image *VisualEntity::GetImage()
{
	return DDraw->GetImage(image);
}

ImageID VisualEntity::GetImageID()
{
	return image;
}
/************************************************************************/

AnimatedEntity::AnimatedEntity() : Entity() {}

AnimatedEntity::AnimatedEntity(U16 x, U16 y, U16 w, U16 h, U8 framecount, char *image) 
	: Entity(x, y, w, h)
{
	if( image )
		Load(image, framecount);
}

AnimatedEntity::~AnimatedEntity()
{
	delete animation;
}

Animation *AnimatedEntity::GetAnimation()
{
	return animation;
}

void AnimatedEntity::Draw(U32 param)
{
	animation->DrawFrame(dim.x, dim.y, param);
}

void AnimatedEntity::Load(char *image, U8 framecount)
{
	std::string s = image;
	animation = new Animation(0, 0, dim.w, dim.h, framecount, s, DDraw);
}

void AnimatedEntity::SetAnimationSpeed(U8 speed)
{
	animation->SetTimeFrame(speed, -1);
}
