/**
 * @file _httprequestqueue.cpp
 * @brief 
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#include "_httprequestqueue.h"

#include "_httpoperation.h"
#include "_mutex.h"


using namespace LLCoreInt;

namespace LLCore
{

HttpRequestQueue * HttpRequestQueue::sInstance(NULL);


HttpRequestQueue::HttpRequestQueue()
	: RefCounted(true)
{
}


HttpRequestQueue::~HttpRequestQueue()
{
	while (! mQueue.empty())
	{
		HttpOperation * op = mQueue.back();
		mQueue.pop_back();
		op->release();
	}
}


void HttpRequestQueue::init()
{
	llassert_always(! sInstance);
	sInstance = new HttpRequestQueue();
}


void HttpRequestQueue::term()
{
	if (sInstance)
	{
		sInstance->release();
		sInstance = NULL;
	}
}


void HttpRequestQueue::addOp(HttpOperation * op)
{
	{
		HttpScopedLock lock(mQueueMutex);

		mQueue.push_back(op);
	}
	mQueueCV.notify_all();
}


HttpOperation * HttpRequestQueue::fetchOp(bool wait)
{
	HttpOperation * result(NULL);

	{
		HttpScopedLock lock(mQueueMutex);

		while (mQueue.empty())
		{
			if (! wait)
				return NULL;
			mQueueCV.wait(lock);
		}

		result = mQueue.front();
		mQueue.erase(mQueue.begin());
	}

	// Caller also acquires the reference count
	return result;
}


void HttpRequestQueue::fetchAll(bool wait, OpContainer & ops)
{
	// Note:  Should probably test whether we're empty or not here.
	// A target passed in with entries is likely also carrying
	// reference counts and we're going to leak something.
	ops.clear();
	{
		HttpScopedLock lock(mQueueMutex);

		while (mQueue.empty())
		{
			if (! wait)
				return;
			mQueueCV.wait(lock);
		}

		mQueue.swap(ops);
	}

	// Caller also acquires the reference counts on each op.
	return;
}

} // end namespace LLCore
