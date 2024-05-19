#include "PCH/PCH.h"
#include "Task.h"

PTask::PTask()
{
	_bIsRunning   = false;
	_bIsCompelete = false;
}

bool PTask::IsRunning() const
{
	return _bIsRunning;
}

bool PTask::IsWaiting() const
{
	return IsRunning() == false && IsCompelete() == false;
}

bool PTask::IsCompelete() const
{
	return _bIsCompelete;
}

bool PTask::IsValid() const
{
	return _taskDelegate.IsBound();
}

void PTask::DoTask()
{
	_bIsRunning = true;

	_taskDelegate.ExecuteIfBound();

	_bIsCompelete = true;
	_bIsRunning = false;
}

HTaskHandle PTask::CreateHandle() const
{
	return HTaskHandle(SharedWrap(this), &_bIsRunning, &_bIsCompelete);
}

HTaskHandle::HTaskHandle(PSharedPtr<PTask> inTask, const HAtomicBool* pInIsRunning, const HAtomicBool* pInIsCompelete)
{
	task = inTask;
	pIsRunning = pInIsRunning;
	pIsCompelete = pInIsCompelete;
}

bool HTaskHandle::IsValid() const
{
	return task.IsValid() && pIsRunning != nullptr;
}

bool HTaskHandle::IsRunning() const
{
	if (IsValid() == false)
	{
		return false;
	}

	return (*pIsRunning) == true;
}

bool HTaskHandle::IsCompelete() const
{
	if (IsValid() == false)
	{
		return false;
	}

	return (*pIsCompelete) == true;
}

bool HTaskHandle::IsWaiting() const
{
	if (IsValid() == false)
	{
		return false;
	}

	return IsRunning() == false && IsCompelete() == false;
}

void HTaskHandle::Wait()
{
	while (IsRunning()) { }
}