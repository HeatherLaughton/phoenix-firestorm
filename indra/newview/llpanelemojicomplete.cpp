﻿/**
* @file llpanelemojicomplete.h
* @brief Header file for LLPanelEmojiComplete
*
* $LicenseInfo:firstyear=2012&license=lgpl$
* Second Life Viewer Source Code
* Copyright (C) 2011, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#include "llemojidictionary.h"
#include "llpanelemojicomplete.h"
#include "lluictrlfactory.h"

constexpr U32 MIN_MOUSE_MOVE_DELTA = 4;

// ============================================================================
// LLPanelEmojiComplete
//

static LLDefaultChildRegistry::Register<LLPanelEmojiComplete> r("emoji_complete");

LLPanelEmojiComplete::Params::Params()
	: selected_image("selected_image")
{
}

LLPanelEmojiComplete::LLPanelEmojiComplete(const LLPanelEmojiComplete::Params& p)
	: LLUICtrl(p)
	, mSelectedImage(p.selected_image)
{
	setFont(p.font);
}

LLPanelEmojiComplete::~LLPanelEmojiComplete()
{
}

void LLPanelEmojiComplete::draw()
{
	if (!mEmojis.empty())
	{
		const S32 centerY = mRenderRect.getCenterY();
		const size_t firstVisibleIdx = mScrollPos, lastVisibleIdx = llmin(mScrollPos + mVisibleEmojis, mEmojis.size()) - 1;

		if (mCurSelected >= firstVisibleIdx && mCurSelected <= lastVisibleIdx)
		{
			const S32 emoji_left = mRenderRect.mLeft + (mCurSelected - firstVisibleIdx) * mEmojiWidth;
			const S32 emoji_height = mFont->getLineHeight() + mPadding;
			mSelectedImage->draw(emoji_left, centerY - emoji_height / 2, mEmojiWidth, emoji_height);
		}

		U32 left = mRenderRect.mLeft + mPadding;
		for (U32 curIdx = firstVisibleIdx; curIdx <= lastVisibleIdx; curIdx++)
		{
			mFont->render(
				mEmojis, curIdx,
				left, centerY,
				LLColor4::white, LLFontGL::LEFT, LLFontGL::VCENTER, LLFontGL::NORMAL, LLFontGL::DROP_SHADOW_SOFT,
				1, S32_MAX, nullptr, false, true);
			left += mEmojiWidth;
		}
	}
}

BOOL LLPanelEmojiComplete::handleHover(S32 x, S32 y, MASK mask)
{
	LLVector2 curHover(x, y);
	if ((mLastHover - curHover).lengthSquared() > MIN_MOUSE_MOVE_DELTA)
	{
		mCurSelected = posToIndex(x, y);
		mLastHover = curHover;
	}

	return TRUE;
}

BOOL LLPanelEmojiComplete::handleKey(KEY key, MASK mask, BOOL called_from_parent)
{
	if (MASK_NONE == mask)
	{
		bool handled = false;
		switch (key)
		{
			case KEY_LEFT:
			case KEY_UP:
				selectPrevious();
				handled = true;
				break;
			case KEY_RIGHT:
			case KEY_DOWN:
				selectNext();
				handled = true;
				break;
		}
		return handled;
	}

	return false;
}

void LLPanelEmojiComplete::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLUICtrl::reshape(width, height, called_from_parent);
	updateConstraints();
}

void LLPanelEmojiComplete::setEmojiHint(const std::string& hint)
{
	mEmojis = LLEmojiDictionary::instance().findMatchingEmojis(hint);
	mScrollPos = llmin(mScrollPos, mEmojis.size());
	updateConstraints();
}

size_t LLPanelEmojiComplete::posToIndex(S32 x, S32 y) const
{
	if (mRenderRect.pointInRect(x, y))
	{
		return llmin((size_t)x / mEmojiWidth, mEmojis.size() - 1);
	}
	return npos;
}

void LLPanelEmojiComplete::select(size_t emoji_idx)
{
	mCurSelected = llclamp<size_t>(emoji_idx, 0, mEmojis.size());
	updateScrollPos();
}

void LLPanelEmojiComplete::selectNext()
{
	select(mCurSelected + 1 < mEmojis.size() ? mCurSelected + 1 : 0);
}

void LLPanelEmojiComplete::selectPrevious()
{
	select(mCurSelected - 1 >= 0 ? mCurSelected - 1 : mEmojis.size() - 1);
}

void LLPanelEmojiComplete::setFont(const LLFontGL* fontp)
{
	mFont = fontp;
	updateConstraints();
}

void LLPanelEmojiComplete::updateConstraints()
{
	const S32 ctrlWidth = getLocalRect().getWidth();

	mEmojiWidth = mFont->getWidthF32(u8"\U0001F431") + mPadding * 2;
	mVisibleEmojis = ctrlWidth / mEmojiWidth;
	mRenderRect = getLocalRect().stretch((ctrlWidth - mVisibleEmojis * mEmojiWidth) / -2, 0);

	updateScrollPos();
}

void LLPanelEmojiComplete::updateScrollPos()
{
	const size_t cntEmoji = mEmojis.size();
	if (0 == cntEmoji || cntEmoji < mVisibleEmojis || 0 == mCurSelected)
	{
		mScrollPos = 0;
	}
	else if (cntEmoji - 1 == mCurSelected)
	{
		mScrollPos = mCurSelected - mVisibleEmojis + 1;
	}
	else
	{
		mScrollPos = mCurSelected - ((float)mCurSelected / (cntEmoji - 2) * (mVisibleEmojis - 2));
	}
}

// ============================================================================
// LLFloaterEmojiComplete
//

LLFloaterEmojiComplete::LLFloaterEmojiComplete(const LLSD& sdKey)
	: LLFloater(sdKey)
{
	// This floater should hover on top of our dependent (with the dependent having the focus)
	setFocusStealsFrontmost(false);
	setAutoFocus(false);
	setBackgroundVisible(false);
}

void LLFloaterEmojiComplete::onOpen(const LLSD& key)
{
	findChild<LLPanelEmojiComplete>("emoji_complete_ctrl")->setEmojiHint(key["hint"].asString());
}

// ============================================================================
