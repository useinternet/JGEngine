#include "PCH/PCH.h"
#include "Core/Transform.h"

HTransform::HTransform(const HVector3& inPosition, const HQuaternion& inRotation, const HVector3& inScale)
	: Position(inPosition)
	, Rotation(inRotation)
	, Scale(inScale)
{
}

HTransform HTransform::Identity()
{
	return HTransform();
}

HMatrix HTransform::ToMatrix() const
{
	return HMatrix::AffineTransformation(Position, Rotation, Scale);
}

HMatrix HTransform::ToWorldMatrix(const HMatrix& parentWorld) const
{
	return ToMatrix() * parentWorld;
}

bool HTransform::operator==(const HTransform& rhs) const
{
	return Position == rhs.Position && Rotation == rhs.Rotation && Scale == rhs.Scale;
}

bool HTransform::operator!=(const HTransform& rhs) const
{
	return !(*this == rhs);
}

void HTransform::WriteJson(PJsonData& json) const
{
	json.AddMember("Position", Position);
	json.AddMember("Rotation", Rotation);
	json.AddMember("Scale", Scale);
}

void HTransform::ReadJson(const PJsonData& json)
{
	json.GetData("Position", &Position);
	json.GetData("Rotation", &Rotation);
	json.GetData("Scale", &Scale);
}
