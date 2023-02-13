/*
 *
 *  co.h
 * 
 *  Color definitions for the entire program. Defines the color schemes we use
 *  throughout the app. Eventually we might want to make this customizable by
 *  the user.
 * 
 */
#pragma once

/*  standard colors */

const ColorF coBlack(0, 0, 0);
const ColorF coWhite(1, 1, 1);
const ColorF coRed(1, 0, 0);
const ColorF coBlue(0, 0, 1);
const ColorF coGreen(0.1f, 0.9f, 0.2f);

/* default colors for UI elements */

const ColorF coStdBack(1, 1, 1);
const ColorF coStdText(0.4f, 0.4f, 0.4f);
const ColorF coHiliteBack(1, 1, 0.4f);

const ColorF coGameBack(0.5f, 0.5f, 0.5f);
const ColorF coGameText(0.6f, 0.6f, 0.6f);

const ColorF coGridLine(0.75f, 0.75f, 0.75f);

const ColorF coScrollBack(.93f, .93f, .93f);

const ColorF coTipBack(1.0f, 1.0f, 0.85f);

const ColorF coButtonBarBack(0.92f, 0.91f, 0.85f);
const ColorF coBtnText(0, 0, 0);
const ColorF coBtnHilite(1, 0, 0);
const ColorF coBtnDisabled(0.75f, 0.75f, 0.75f);

const ColorF coStaticText(0, 0, 0);

/*  board colors */

const ColorF coBoardDark(0.42f, 0.54f, 0.32f);
const ColorF coBoardLight(1.0f, 0.98f, 0.95f);
const ColorF coBoardBWDark(0.5f, 0.5, 0.5f);
const ColorF coBoardBWLight(0.9f, 0.9f, 0.9f);
const ColorF coBoardMoveHilite(1.0f, 0.5f, 0);
const float opacityBoardMoveHilite = 0.1f;
const ColorF coAnnotation(0.15f, 0.8f, 0.25f);
const float opacityAnnotation = 0.75f;
const ColorF coDragDelCircle(0.65f, 0.15f, 0.25f);
const ColorF coDragDelCross(0, 0, 0);

/*  clocks */

const ColorF coClockText(0.5f, 0.9f, 1.0f);
const ColorF coClockBack(0.2f, 0.2f, 0.2f);
const ColorF coClockWarningText(0.9f, 0.2f, 0.2f);
const ColorF coClockWarningBack = coClockBack;
const ColorF coClockTCText = coClockText;
const ColorF coClockTCBack = coClockBack;
const ColorF coClockTCCurText = coWhite;
const ColorF coClockTCCurBack = coClockBack;
const ColorF coClockFlag = coClockWarningText;
/*
const ColorF coClockText(0.0f, 0.25f, 0.45f);
const ColorF coClockBack(0.86f, 0.90f, 0.86f);
const ColorF coClockWarningText = coClockText;
const ColorF coClockWarningBack(0.95f, 0.80f, 0.20f);
const ColorF coClockTCText = coClockText;
const ColorF coClockTCBack = coClockBack;
const ColorF coClockTCCurText = coClockText;
const ColorF coClockTCCurBack = coClockWarningBack;
const ColorF coClockFlag(1.0f, 0.0f, 0.0f);
*/
