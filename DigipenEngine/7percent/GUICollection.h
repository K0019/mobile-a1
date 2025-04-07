/******************************************************************************/
/*!
\file   GUICollection.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   12/02/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is an interface file that implements an intermediate layer connecting engine
  code with ImGui code, hiding ImGui stuff that shouldn't be exposed and alleviating
  the need for engine code to #ifdef out GUI code depending on the project configuration.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include "Utilities.h"
#include "MacroTemplates.h"
#include <string>
#ifdef IMGUI_ENABLED
#include "imgui_internal.h"
#include "imgui.h"
#else
#include "percentmath.h"
#include <type_traits>
#endif

namespace gui {

#pragma region Types

#ifdef IMGUI_ENABLED
	using Vec2 = ImVec2;
	using Vec4 = ImVec4;
#else
	using Vec2 = Vector2;
	using Vec4 = Vector4;
#endif

	namespace types
	{
#ifdef IMGUI_ENABLED
		using InputTextCallbackData = ImGuiInputTextCallbackData;
		using InputTextCallback = ImGuiInputTextCallback;
#else
		using InputTextCallbackData = struct {};
		using InputTextCallback = int(*)(InputTextCallbackData*);
#endif
	}

#pragma endregion // Types

#pragma region Flags

	//! ImGuiKey
	// TODO: Finish this
#define GUICOLLECTION_KEY \
	X(UP, ImGuiKey::ImGuiKey_UpArrow) \
	X(DOWN, ImGuiKey::ImGuiKey_DownArrow) \
	X(ENTER, ImGuiKey::ImGuiKey_Enter) \

	//! ImGuiWindowFlags
#define GUICOLLECTION_FLAG_WINDOW \
	X(NONE, ImGuiWindowFlags_None) \
	X(NO_TITLE_BAR, ImGuiWindowFlags_NoTitleBar) \
	X(NO_RESIZE, ImGuiWindowFlags_NoResize) \
	X(NO_MOVE, ImGuiWindowFlags_NoMove) \
	X(NO_SCROLL_BAR, ImGuiWindowFlags_NoScrollbar) \
	X(NO_SCROLL_WITH_MOUSE, ImGuiWindowFlags_NoScrollWithMouse) \
	X(NO_COLLAPSE, ImGuiWindowFlags_NoCollapse) \
	X(ALWAYS_AUTO_RESIZE, ImGuiWindowFlags_AlwaysAutoResize) \
	X(NO_BACKGROUND, ImGuiWindowFlags_NoBackground) \
	X(NO_SAVED_SETTINGS, ImGuiWindowFlags_NoSavedSettings) \
	X(NO_MOUSE_INPUTS, ImGuiWindowFlags_NoMouseInputs) \
	X(HAS_MENU_BAR, ImGuiWindowFlags_MenuBar) \
	X(HORIZONTAL_SCROLL_BAR, ImGuiWindowFlags_HorizontalScrollbar) \
	X(NO_FOCUS_ON_APPEARING, ImGuiWindowFlags_NoFocusOnAppearing) \
	X(NO_BRING_TO_FRONT_ON_FOCUS, ImGuiWindowFlags_NoBringToFrontOnFocus) \
	X(ALWAYS_VERTICAL_SCROLL_BAR, ImGuiWindowFlags_AlwaysVerticalScrollbar) \
	X(ALWAYS_HORIZONTAL_SCROLL_BAR, ImGuiWindowFlags_AlwaysHorizontalScrollbar) \
	X(NO_NAV_INPUTS, ImGuiWindowFlags_NoNavInputs) \
	X(NO_NAV_FOCUS, ImGuiWindowFlags_NoNavFocus) \
	X(UNSAVED_DOCUMENT, ImGuiWindowFlags_UnsavedDocument) \
	X(NO_DOCKING, ImGuiWindowFlags_NoDocking)
	// Additional (see enum class definition): NO_NAV, NO_DECORATION, NO_INPUTS

	//! ImGuiChildFlags
#define GUICOLLECTION_FLAG_CHILD \
	X(NONE, ImGuiChildFlags_None) \
	X(BORDERS, ImGuiChildFlags_Borders) \
	X(ALWAYS_USE_WINDOW_PADDING, ImGuiChildFlags_AlwaysUseWindowPadding) \
	X(RESIZE_X, ImGuiChildFlags_ResizeX) \
	X(RESIZE_Y, ImGuiChildFlags_ResizeY) \
	X(AUTO_RESIZE_X, ImGuiChildFlags_AutoResizeX) \
	X(AUTO_RESIZE_Y, ImGuiChildFlags_AutoResizeY) \
	X(ALWAYS_AUTO_RESIZE, ImGuiChildFlags_AlwaysAutoResize) \
	X(FRAME_STYLE, ImGuiChildFlags_FrameStyle)

	//! ImGuiInputTextFlags
#define GUICOLLECTION_FLAG_INPUT_TEXT \
	X(NONE, ImGuiInputTextFlags_None) \
	X(CHARS_DECIMAL, ImGuiInputTextFlags_CharsDecimal) \
	X(CHARS_HEXADECIMAL, ImGuiInputTextFlags_CharsHexadecimal) \
	X(CHARS_SCIENTIFIC, ImGuiInputTextFlags_CharsScientific) \
	X(CHARS_UPPERCASE, ImGuiInputTextFlags_CharsUppercase) \
	X(CHARS_NO_BLANK, ImGuiInputTextFlags_CharsNoBlank) \
	X(ALLOW_TAB_INPUT, ImGuiInputTextFlags_AllowTabInput) \
	X(ENTER_RETURNS_TRUE, ImGuiInputTextFlags_EnterReturnsTrue) \
	X(ESCAPE_CLEARS_ALL, ImGuiInputTextFlags_EscapeClearsAll) \
	X(CTRL_ENTER_FOR_NEW_LINE, ImGuiInputTextFlags_CtrlEnterForNewLine) \
	X(READ_ONLY, ImGuiInputTextFlags_ReadOnly) \
	X(PASSWORD, ImGuiInputTextFlags_Password) \
	X(ALWAYS_OVERWRITE, ImGuiInputTextFlags_AlwaysOverwrite) \
	X(AUTO_SELECT_ALL, ImGuiInputTextFlags_AutoSelectAll) \
	X(PARSE_EMPTY_REF_VAL, ImGuiInputTextFlags_ParseEmptyRefVal) \
	X(DISPLAY_EMPTY_REF_VAL, ImGuiInputTextFlags_DisplayEmptyRefVal) \
	X(NO_HORIZONTAL_SCROLL, ImGuiInputTextFlags_NoHorizontalScroll) \
	X(NO_UNDO_REDO, ImGuiInputTextFlags_NoUndoRedo) \
	X(CALLBACK_COMPLETION, ImGuiInputTextFlags_CallbackCompletion) \
	X(CALLBACK_HISTORY, ImGuiInputTextFlags_CallbackHistory) \
	X(CALLBACK_ALWAYS, ImGuiInputTextFlags_CallbackAlways) \
	X(CALLBACK_CHAR_FILTER, ImGuiInputTextFlags_CallbackCharFilter) \
	X(CALLBACK_RESIZE, ImGuiInputTextFlags_CallbackResize) \
	X(CALLBACK_EDIT, ImGuiInputTextFlags_CallbackEdit)

	//! ImGuiTreeNodeFlags
#define GUICOLLECTION_FLAG_TREE_NODE \
	X(NONE, ImGuiTreeNodeFlags_None) \
	X(SELECTED, ImGuiTreeNodeFlags_Selected) \
	X(FRAMED, ImGuiTreeNodeFlags_Framed) \
	X(ALLOW_OVERLAP, ImGuiTreeNodeFlags_AllowOverlap) \
	X(NO_TREE_PUSH_ON_OPEN, ImGuiTreeNodeFlags_NoTreePushOnOpen) \
	X(NO_AUTO_OPEN_ON_LOG, ImGuiTreeNodeFlags_NoAutoOpenOnLog) \
	X(DEFAULT_OPEN, ImGuiTreeNodeFlags_DefaultOpen) \
	X(OPEN_ON_DOUBLE_CLICK, ImGuiTreeNodeFlags_OpenOnDoubleClick) \
	X(OPEN_ON_ARROW, ImGuiTreeNodeFlags_OpenOnArrow) \
	X(LEAF, ImGuiTreeNodeFlags_Leaf) \
	X(BULLET, ImGuiTreeNodeFlags_Bullet) \
	X(FRAME_PADDING, ImGuiTreeNodeFlags_FramePadding) \
	X(SPAN_AVAILABLE_WIDTH, ImGuiTreeNodeFlags_SpanAvailWidth) \
	X(SPAN_FULL_WIDTH, ImGuiTreeNodeFlags_SpanFullWidth) \
	X(SPAN_TEXT_WIDTH, ImGuiTreeNodeFlags_SpanTextWidth) \
	X(SPAN_ALL_COLUMNS, ImGuiTreeNodeFlags_SpanAllColumns) \
	X(NAV_LEFT_JUMPS_BACK_HERE, ImGuiTreeNodeFlags_NavLeftJumpsBackHere) \
	X(COLLAPSING_HEADER, ImGuiTreeNodeFlags_CollapsingHeader)

	//! ImGuiPopupFlags
#define GUICOLLECTION_FLAG_POPUP \
	X(NONE, ImGuiPopupFlags_None) \
	X(MOUSE_LEFT, ImGuiPopupFlags_MouseButtonLeft) \
	X(MOUSE_RIGHT, ImGuiPopupFlags_MouseButtonRight) \
	X(MOUSE_MIDDLE, ImGuiPopupFlags_MouseButtonMiddle) \
	X(MOUSE_ANY, ImGuiPopupFlags_MouseButtonMask_) \
	X(NO_REOPEN, ImGuiPopupFlags_NoReopen) \
	X(NO_OPEN_OVER_EXISTING_POPUP, ImGuiPopupFlags_NoOpenOverExistingPopup) \
	X(NO_OPEN_OVER_ITEMS, ImGuiPopupFlags_NoOpenOverItems) \
	X(ANY_POPUP_ID, ImGuiPopupFlags_AnyPopupId) \
	X(ANY_POPUP_LEVEL, ImGuiPopupFlags_AnyPopupLevel) \
	X(ANY_POPUP, ImGuiPopupFlags_AnyPopup)

	//! ImGuiComboFlags
#define GUICOLLECTION_FLAG_COMBO \
	X(NONE, ImGuiComboFlags_None) \
	X(POPUP_ALIGN_LEFT, ImGuiComboFlags_PopupAlignLeft) \
	X(HEIGHT_SMALL, ImGuiComboFlags_HeightSmall) \
	X(HEIGHT_REGULAR, ImGuiComboFlags_HeightRegular) \
	X(HEIGHT_LARGE, ImGuiComboFlags_HeightLarge) \
	X(HEIGHT_LARGEST, ImGuiComboFlags_HeightLargest) \
	X(NO_ARROW_BUTTON, ImGuiComboFlags_NoArrowButton) \
	X(NO_PREVIEW, ImGuiComboFlags_NoPreview) \
	X(WIDTH_FIT_PREVIEW, ImGuiComboFlags_WidthFitPreview)

	//! ImGuiCol
#define GUICOLLECTION_FLAG_COLOR \
	X(TEXT, ImGuiCol_Text) \
	X(TEXT_DISABLED, ImGuiCol_TextDisabled) \
	X(WINDOW_BG, ImGuiCol_WindowBg) \
	X(CHILD_BG, ImGuiCol_ChildBg) \
	X(POPUP_BG, ImGuiCol_PopupBg) \
	X(BORDER, ImGuiCol_Border) \
	X(BORDER_SHADOW, ImGuiCol_BorderShadow) \
	X(FRAME_BG, ImGuiCol_FrameBg) \
	X(FRAME_BG_HOVERED, ImGuiCol_FrameBgHovered) \
	X(FRAME_BG_ACTIVE, ImGuiCol_FrameBgActive) \
	X(TITLE_BG, ImGuiCol_TitleBg) \
	X(TITLE_BG_ACTIVE, ImGuiCol_TitleBgActive) \
	X(TITLE_BG_COLLAPSED, ImGuiCol_TitleBgCollapsed) \
	X(MENU_BAR_BG, ImGuiCol_MenuBarBg) \
	X(SCROLL_BAR_BG, ImGuiCol_ScrollbarBg) \
	X(SCROLL_BAR_GRAB, ImGuiCol_ScrollbarGrab) \
	X(SCROLL_BAR_GRAB_HOVERED, ImGuiCol_ScrollbarGrabHovered) \
	X(SCROLL_BAR_GRAB_ACTIVE, ImGuiCol_ScrollbarGrabActive) \
	X(CHECK_MARK, ImGuiCol_CheckMark) \
	X(SLIDER_GRAB, ImGuiCol_SliderGrab) \
	X(SLIDER_GRAB_ACTIVE, ImGuiCol_SliderGrabActive) \
	X(BUTTON, ImGuiCol_Button) \
	X(BUTTON_HOVERED, ImGuiCol_ButtonHovered) \
	X(BUTTON_ACTIVE, ImGuiCol_ButtonActive) \
	X(HEADER, ImGuiCol_Header) \
	X(HEADER_HOVERED, ImGuiCol_HeaderHovered) \
	X(HEADER_ACTIVE, ImGuiCol_HeaderActive) \
	X(SEPARATOR, ImGuiCol_Separator) \
	X(SEPARATOR_HOVERED, ImGuiCol_SeparatorHovered) \
	X(SEPARATOR_ACTIVE, ImGuiCol_SeparatorActive) \
	X(RESIZE_GRIP, ImGuiCol_ResizeGrip) \
	X(RESIZE_GRIP_HOVERED, ImGuiCol_ResizeGripHovered) \
	X(RESIZE_GRIP_ACTIVE, ImGuiCol_ResizeGripActive) \
	X(TAB_HOVERED, ImGuiCol_TabHovered) \
	X(TAB, ImGuiCol_Tab) \
	X(TAB_SELECTED, ImGuiCol_TabSelected) \
	X(TAB_SELECTED_OVERLINE, ImGuiCol_TabSelectedOverline) \
	X(TAB_DIMMED, ImGuiCol_TabDimmed) \
	X(TAB_DIMMED_SELECTED, ImGuiCol_TabDimmedSelected) \
	X(TAB_DIMMED_SELECTED_OVERLINE, ImGuiCol_TabDimmedSelectedOverline) \
	X(DOCKING_PREVIEW, ImGuiCol_DockingPreview) \
	X(DOCKING_EMPTY_BG, ImGuiCol_DockingEmptyBg) \
	X(PLOT_LINES, ImGuiCol_PlotLines) \
	X(PLOT_LINES_HOVERED, ImGuiCol_PlotLinesHovered) \
	X(PLOT_HISTOGRAM, ImGuiCol_PlotHistogram) \
	X(PLOT_HISTOGRAM_HOVERED, ImGuiCol_PlotHistogramHovered) \
	X(TABLE_HEADER_BG, ImGuiCol_TableHeaderBg) \
	X(TABLE_BORDER_STRONG, ImGuiCol_TableBorderStrong) \
	X(TABLE_BORDER_LIGHT, ImGuiCol_TableBorderLight) \
	X(TABLE_ROW_BG_EVEN_ROWS, ImGuiCol_TableRowBg) \
	X(TABLE_ROW_BG_ODD_ROWS, ImGuiCol_TableRowBgAlt) \
	X(TEXT_LINK, ImGuiCol_TextLink) \
	X(TEXT_SELECTED_BG, ImGuiCol_TextSelectedBg) \
	X(DRAG_DROP_TARGET, ImGuiCol_DragDropTarget) \
	X(NAV_HIGHLIGHT, ImGuiCol_NavHighlight) \
	X(NAV_WINDOWING_HIGHLIGHT, ImGuiCol_NavWindowingHighlight) \
	X(NAV_WINDOWING_DIM_BG, ImGuiCol_NavWindowingDimBg) \
	X(MODAL_WINDOW_DIM_BG, ImGuiCol_ModalWindowDimBg)

	//! ImGuiStyleVar
#define GUICOLLECTION_FLAG_STYLE_VAR \
	X(ALPHA, ImGuiStyleVar_Alpha) \
	X(DISABLED_ALPHA, ImGuiStyleVar_DisabledAlpha) \
	X(WINDOW_PADDING, ImGuiStyleVar_WindowPadding) \
	X(WINDOW_ROUNDING, ImGuiStyleVar_WindowRounding) \
	X(WINDOW_BORDER_SIZE, ImGuiStyleVar_WindowBorderSize) \
	X(WINDOW_MIN_SIZE, ImGuiStyleVar_WindowMinSize) \
	X(WINDOW_TITLE_ALIGN, ImGuiStyleVar_WindowTitleAlign) \
	X(CHILD_ROUNDING, ImGuiStyleVar_ChildRounding) \
	X(CHILD_BORDER_SIZE, ImGuiStyleVar_ChildBorderSize) \
	X(POPUP_ROUNDING, ImGuiStyleVar_PopupRounding) \
	X(POPUP_BORDER_SIZE, ImGuiStyleVar_PopupBorderSize) \
	X(FRAME_PADDING, ImGuiStyleVar_FramePadding) \
	X(FRAME_ROUNDING, ImGuiStyleVar_FrameRounding) \
	X(FRAME_BORDER_SIZE, ImGuiStyleVar_FrameBorderSize) \
	X(ITEM_SPACING, ImGuiStyleVar_ItemSpacing) \
	X(ITEM_INNER_SPACING, ImGuiStyleVar_ItemInnerSpacing) \
	X(INDENT_SPACING, ImGuiStyleVar_IndentSpacing) \
	X(CELL_PADDING, ImGuiStyleVar_CellPadding) \
	X(SCROLL_BAR_SIZE, ImGuiStyleVar_ScrollbarSize) \
	X(SCROLL_BAR_ROUNDING, ImGuiStyleVar_ScrollbarRounding) \
	X(GRAB_MIN_SIZE, ImGuiStyleVar_GrabMinSize) \
	X(GRAB_ROUNDING, ImGuiStyleVar_GrabRounding) \
	X(TAB_ROUNDING, ImGuiStyleVar_TabRounding) \
	X(TAB_BORDER_SIZE, ImGuiStyleVar_TabBorderSize) \
	X(TAB_BAR_BORDER_SIZE, ImGuiStyleVar_TabBarBorderSize) \
	X(TAB_BAR_OVERLINE_SIZE, ImGuiStyleVar_TabBarOverlineSize) \
	X(TABLE_ANGLED_HEADERS_ANGLE, ImGuiStyleVar_TableAngledHeadersAngle) \
	X(TABLE_ANGLED_HEADERS_TEXT_ALIGN, ImGuiStyleVar_TableAngledHeadersTextAlign) \
	X(BUTTON_TEXT_ALIGN, ImGuiStyleVar_ButtonTextAlign) \
	X(SELECTABLE_TEXT_ALIGN, ImGuiStyleVar_SelectableTextAlign) \
	X(SEPARATOR_TEXT_BORDER_SIZE, ImGuiStyleVar_SeparatorTextBorderSize) \
	X(SEPARATOR_TEXT_ALIGN, ImGuiStyleVar_SeparatorTextAlign) \
	X(SEPARATOR_TEXT_TEXT_PADDING, ImGuiStyleVar_SeparatorTextPadding) \
	X(DOCKING_SEPARATOR_SIZE, ImGuiStyleVar_DockingSeparatorSize)

	//! ImGuiCond
#define GUICOLLECTION_FLAG_COND \
	X(NONE, ImGuiCond_None) \
	X(ONCE, ImGuiCond_Once) \
	X(FIRST_USE_EVER, ImGuiCond_FirstUseEver) \
	X(APPEARING, ImGuiCond_Appearing)

	//! ImGuiTableFlags
#define GUICOLLECTION_FLAG_TABLE \
	X(NONE, ImGuiTableFlags_None) \
	X(RESIZABLE, ImGuiTableFlags_Resizable) \
	X(REORDERABLE, ImGuiTableFlags_Reorderable) \
	X(HIDEABLE, ImGuiTableFlags_Hideable) \
	X(SORTABLE, ImGuiTableFlags_Sortable) \
	X(NO_SAVED_SETTINGS, ImGuiTableFlags_NoSavedSettings) \
	X(CONTEXT_MENU_IN_BODY, ImGuiTableFlags_ContextMenuInBody) \
	X(ROW_BG, ImGuiTableFlags_RowBg) \
	X(BORDERS_INNER_H, ImGuiTableFlags_BordersInnerH) \
	X(BORDERS_OUTER_H, ImGuiTableFlags_BordersOuterH) \
	X(BORDERS_INNER_V, ImGuiTableFlags_BordersInnerV) \
	X(BORDERS_OUTER_V, ImGuiTableFlags_BordersOuterV) \
	X(BORDERS_H, ImGuiTableFlags_BordersH) \
	X(BORDERS_V, ImGuiTableFlags_BordersV) \
	X(BORDERS_INNER, ImGuiTableFlags_BordersInner) \
	X(BORDERS_OUTER, ImGuiTableFlags_BordersOuter) \
	X(BORDERS, ImGuiTableFlags_Borders) \
	X(NO_BORDERS_IN_BODY, ImGuiTableFlags_NoBordersInBody) \
	X(NO_BORDERS_IN_BODY_UNTIL_RESIZE, ImGuiTableFlags_NoBordersInBodyUntilResize) \
	X(SIZING_FIXED_FIT, ImGuiTableFlags_SizingFixedFit) \
	X(SIZING_FIXED_SAME, ImGuiTableFlags_SizingFixedSame) \
	X(SIZING_STRETCH_PROPORTIONAL, ImGuiTableFlags_SizingStretchProp) \
	X(SIZING_STRETCH_SAME, ImGuiTableFlags_SizingStretchSame) \
	X(NO_HOST_EXTEND_X, ImGuiTableFlags_NoHostExtendX) \
	X(NO_HOST_EXTEND_Y, ImGuiTableFlags_NoHostExtendY) \
	X(NO_KEEP_COLUMNS_VISIBLE, ImGuiTableFlags_NoKeepColumnsVisible) \
	X(PRECISE_WIDTHS, ImGuiTableFlags_PreciseWidths) \
	X(NO_CLIP, ImGuiTableFlags_NoClip) \
	X(PAD_OUTER_X, ImGuiTableFlags_PadOuterX) \
	X(NO_PAD_OUTER_X, ImGuiTableFlags_NoPadOuterX) \
	X(NO_PAD_INNER_X, ImGuiTableFlags_NoPadInnerX) \
	X(SCROLL_X, ImGuiTableFlags_ScrollX) \
	X(SORT_MULTI, ImGuiTableFlags_SortMulti) \
	X(SORT_TRISTATE, ImGuiTableFlags_SortTristate) \
	X(HIGHLIGHT_HOVERED_COLUMN, ImGuiTableFlags_HighlightHoveredColumn) \
	X(HIDE_HEADER, ImGuiTableFlags_HighlightHoveredColumn << 1) // Custom flag, disables the row header (doesn't call TableHeadersRow()).

	//! ImGuiTableColumnFlags
#define GUICOLLECTION_FLAG_TABLE_COLUMN \
	X(NONE, ImGuiTableColumnFlags_None) \
	X(DISABLED, ImGuiTableColumnFlags_Disabled) \
	X(DEFAULT_HIDE, ImGuiTableColumnFlags_DefaultHide) \
	X(DEFAULT_SORT, ImGuiTableColumnFlags_DefaultSort) \
	X(WIDTH_STRETCH, ImGuiTableColumnFlags_WidthStretch) \
	X(WIDTH_FIXED, ImGuiTableColumnFlags_WidthFixed) \
	X(NO_RESIZE, ImGuiTableColumnFlags_NoResize) \
	X(NO_REORDER, ImGuiTableColumnFlags_NoReorder) \
	X(NO_HIDE, ImGuiTableColumnFlags_NoHide) \
	X(NO_CLIP, ImGuiTableColumnFlags_NoClip) \
	X(NO_SORT, ImGuiTableColumnFlags_NoSort) \
	X(NO_SORT_ASCENDING, ImGuiTableColumnFlags_NoSortAscending) \
	X(NO_SORT_DESCENDING, ImGuiTableColumnFlags_NoSortDescending) \
	X(NO_HEADER_LABEL, ImGuiTableColumnFlags_NoHeaderLabel) \
	X(NO_HEADER_WIDTH, ImGuiTableColumnFlags_NoHeaderWidth) \
	X(PREFER_SORT_ASCENDING, ImGuiTableColumnFlags_PreferSortAscending) \
	X(PREFER_SORT_DESCENDING, ImGuiTableColumnFlags_PreferSortDescending) \
	X(ENABLE_INDENT, ImGuiTableColumnFlags_IndentEnable) \
	X(DISABLE_INDENT, ImGuiTableColumnFlags_IndentDisable) \
	X(ANGLED_HEADER, ImGuiTableColumnFlags_AngledHeader) \
	X(IS_ENABLED, ImGuiTableColumnFlags_IsEnabled) \
	X(IS_VISIBLE, ImGuiTableColumnFlags_IsVisible) \
	X(IS_SORTED, ImGuiTableColumnFlags_IsSorted) \
	X(IS_HOVERED, ImGuiTableColumnFlags_IsHovered)

#ifdef IMGUI_ENABLED
#define X(enumName, value) enumName = value,
#else
#define X(enumName, value) enumName,
#endif

	/*****************************************************************//*!
	\enum class KEY
	\brief
		ImGuiKey
	*//******************************************************************/
	enum class KEY : int {
		GUICOLLECTION_KEY
	};
	GENERATE_ENUM_CLASS_ITERATION_OPERATORS(KEY)

	/*****************************************************************//*!
	\enum class FLAG_WINDOW
	\brief
		ImGuiWindowFlags
	*//******************************************************************/
	enum class FLAG_WINDOW : int {
		GUICOLLECTION_FLAG_WINDOW
		NO_NAV = NO_NAV_INPUTS | NO_NAV_FOCUS,
		NO_DECORATION = NO_TITLE_BAR | NO_RESIZE | NO_SCROLL_BAR | NO_COLLAPSE,
		NO_INPUTS = NO_MOUSE_INPUTS | NO_NAV_INPUTS | NO_NAV_FOCUS
	};
	GENERATE_ENUM_CLASS_BITWISE_OPERATORS(FLAG_WINDOW)

	/*****************************************************************//*!
	\enum class FLAG_CHILD
	\brief
		ImGuiChildFlags
	*//******************************************************************/
	enum class FLAG_CHILD : int {
		GUICOLLECTION_FLAG_CHILD
	};
	GENERATE_ENUM_CLASS_BITWISE_OPERATORS(FLAG_CHILD)

	/*****************************************************************//*!
	\enum class FLAG_INPUT_TEXT
	\brief
		ImGuiInputTextFlags
	*//******************************************************************/
	enum class FLAG_INPUT_TEXT : int {
		GUICOLLECTION_FLAG_INPUT_TEXT
	};
	GENERATE_ENUM_CLASS_BITWISE_OPERATORS(FLAG_INPUT_TEXT)

	/*****************************************************************//*!
	\enum class FLAG_TREE_NODE
	\brief
		ImGuiTreeNodeFlags
	*//******************************************************************/
	enum class FLAG_TREE_NODE : int {
		GUICOLLECTION_FLAG_TREE_NODE
	};
	GENERATE_ENUM_CLASS_BITWISE_OPERATORS(FLAG_TREE_NODE)

	/*****************************************************************//*!
	\enum class FLAG_POPUP
	\brief
		ImGuiPopupFlags
	*//******************************************************************/
	enum class FLAG_POPUP : int {
		GUICOLLECTION_FLAG_POPUP
	};
	GENERATE_ENUM_CLASS_BITWISE_OPERATORS(FLAG_POPUP)

	/*****************************************************************//*!
	\enum class FLAG_COMBO
	\brief
		ImGuiComboFlags
	*//******************************************************************/
	enum class FLAG_COMBO : int {
		GUICOLLECTION_FLAG_COMBO
	};
	GENERATE_ENUM_CLASS_BITWISE_OPERATORS(FLAG_COMBO)

	/*****************************************************************//*!
	\enum class FLAG_STYLE_COLOR
	\brief
		ImGuiCol
	*//******************************************************************/
	enum class FLAG_STYLE_COLOR : int {
		GUICOLLECTION_FLAG_COLOR
	};
	// No bitwise operators for this flag as they aren't supposed to be combined (these aren't binary flags)

	/*****************************************************************//*!
	\enum class FLAG_STYLE_VAR
	\brief
		ImGuiStyleVar
	*//******************************************************************/
	enum class FLAG_STYLE_VAR : int {
		GUICOLLECTION_FLAG_STYLE_VAR
	};
	// No bitwise operators for this flag as they aren't supposed to be combined (these aren't binary flags)

	/*****************************************************************//*!
	\enum class FLAG_COND
	\brief
		ImGuiCond
	*//******************************************************************/
	enum class FLAG_COND : int {
		GUICOLLECTION_FLAG_COND
	};
	// No bitwise operators for this flag as they aren't supposed to be combined (see ImGuiCond_ comment)

	/*****************************************************************//*!
	\enum class FLAG_TABLE
	\brief
		ImGuiTableFlags
	*//******************************************************************/
	enum class FLAG_TABLE : int {
		GUICOLLECTION_FLAG_TABLE
	};
	GENERATE_ENUM_CLASS_BITWISE_OPERATORS(FLAG_TABLE)

	/*****************************************************************//*!
	\enum class FLAG_TABLE_COLUMN
	\brief
		ImGuiTableColumnFlags
	*//******************************************************************/
	enum class FLAG_TABLE_COLUMN : int {
		GUICOLLECTION_FLAG_TABLE_COLUMN
	};
	GENERATE_ENUM_CLASS_BITWISE_OPERATORS(FLAG_TABLE_COLUMN)

#undef X

#pragma endregion // Flags

}

namespace gui {
	namespace internal {

		/*****************************************************************//*!
		\class BeginEndBound
		\brief
			A helper base class automatically providing the logic to call specified
			Begin() and End() functions based on the object's lifetime, so us programmers
			will never forget to call the 2 functions as a pair at the correct timing.
		\tparam StartFunc
			The function called when the object is created.
		\tparam EndFunc
			The function called when the object is destroyed. If nullptr, nothing happens.
		\tparam AlwaysCallEnd
			Whether to always call EndFunc() even if StartFunc() returned a fail value.
		\tparam StartIsOptional
			Whether the first argument passed to args is a bool indicating whether to call StartFunc() or not.
		*//******************************************************************/
		template <auto StartFunc, void (*EndFunc)() = nullptr, bool AlwaysCallEnd = false, bool StartIsOptional = false>
		class BeginEndBound
		{
		public:
			/*****************************************************************//*!
			\brief
				Constructor. Calls StartFunc().
				// Pass by value because most arguments in ImGui are small.
			\tparam Args
				The types of the arguments.
			\tparam args
				The arguments to StartFunc(). If StartIsOptional is true, the first argument
				must be a bool that determines whether to call StartFunc().
			*//******************************************************************/
			template <typename ...Args>
			BeginEndBound(Args... args);

			/*****************************************************************//*!
			\brief
				Destructor.
			*//******************************************************************/
			~BeginEndBound();

			/*****************************************************************//*!
			\brief
				Returns whether StartFunc() succeeded.
			\return
				True if StartFunc() returned true or no value. False if StartFunc() returned false.
			*//******************************************************************/
			operator bool() const;

		private:
			/*****************************************************************//*!
			\brief
				Calls StartFunc() with the provided arguments, if StartFunc() exists.
				// Pass by value because most arguments in ImGui are small.
			\tparam Args
				The types of the arguments.
			\tparam args
				The arguments to StartFunc().
			\return
				The return value of StartFunc(). True if StartFunc() doesn't return a bool.
			*//******************************************************************/
			template <typename ...Args>
			static bool CallStartFunc(Args... args);

			/*****************************************************************//*!
			\brief
				Calls StartFunc() with the provided arguments depending on the template's parameters and provided arguments.
				// Pass by value because most arguments in ImGui are small.
			\tparam Args
				The types of the arguments.
			\tparam args
				The arguments to StartFunc(). If StartIsOptional is true, the first argument must be a bool.
			\return
				The return value of StartFunc(). True if StartFunc() doesn't return a bool.
				If StartIsOptional is true and the first argument is false, returns false.
			*//******************************************************************/
			template <typename ...Args>
			static bool CallStartFuncBasedOnParams(Args... args);

		private:
			//! The value that StartFunc() returned.
			bool isOpen;
		};

		// For disambiguation
#ifdef IMGUI_ENABLED
		//! ImGui::BeginChild()
		bool BeginChild(const char* str_id, const ImVec2& size, ImGuiChildFlags child_flags, ImGuiWindowFlags window_flags);

		//! ImGui::CollapsingHeader()
		bool CollapsingHeader(const char* label, ImGuiTreeNodeFlags flags);

		//! ImGui::PushStyleVar
		void PushStyleVar(ImGuiStyleVar idx, const ImVec2& val);
		//! ImGui::PopStyleVar (remove arguments)
		void PopStyleVar();

		//! ImGui::PushStyleColor
		void PushStyleColor(ImGuiCol idx, const ImVec4& val);
		//! ImGui::PopStyleColor (remove arguments)
		void PopStyleColor();
#endif

#ifdef IMGUI_ENABLED
		using BeginEndBound_Child = BeginEndBound<BeginChild, ImGui::EndChild, true>;
		using BeginEndBound_Popup = BeginEndBound<ImGui::BeginPopup, ImGui::EndPopup>;
		using BeginEndBound_Tooltip = BeginEndBound<ImGui::BeginTooltip, ImGui::EndTooltip, false, true>;

		using BeginEndBound_TextWrapPos = BeginEndBound<ImGui::PushTextWrapPos, ImGui::PopTextWrapPos>;

		using BeginEndBound_Button = BeginEndBound<ImGui::Button>;

		using BeginEndBound_MenuBar = BeginEndBound<ImGui::BeginMenuBar, ImGui::EndMenuBar>;
		using BeginEndBound_Menu = BeginEndBound<ImGui::BeginMenu, ImGui::EndMenu>;
		using BeginEndBound_ItemContextMenu = BeginEndBound<ImGui::BeginPopupContextItem, ImGui::EndPopup>;

		using BeginEndBound_CollapsingHeader = BeginEndBound<CollapsingHeader>;

		using BeginEndBound_Table = BeginEndBound<ImGui::BeginTable, ImGui::EndTable>;

		using BeginEndBound_Group = BeginEndBound<ImGui::BeginGroup, ImGui::EndGroup>;

		using BeginEndBound_StyleVar = BeginEndBound<PushStyleVar, PopStyleVar>;
		using BeginEndBound_StyleColor = BeginEndBound<PushStyleColor, PopStyleColor, false, true>;

		using BeginEndBound_Disabled = BeginEndBound<ImGui::BeginDisabled, ImGui::EndDisabled>;
#else
		using BeginEndBound_Child = std::false_type;
		using BeginEndBound_Popup = std::false_type;
		using BeginEndBound_Tooltip = std::false_type;

		using BeginEndBound_TextWrapPos = std::false_type;

		using BeginEndBound_Button = std::false_type;

		using BeginEndBound_MenuBar = std::false_type;
		using BeginEndBound_Menu = std::false_type;
		using BeginEndBound_ItemContextMenu = std::false_type;

		using BeginEndBound_CollapsingHeader = std::false_type;

		using BeginEndBound_Table = std::false_type;

		using BeginEndBound_Group = std::false_type;

		using BeginEndBound_StyleVar = std::false_type;
		using BeginEndBound_StyleColor = std::false_type;

		using BeginEndBound_Disabled = std::false_type;
#endif

		/*****************************************************************//*!
		\class ContainerBase
		\brief
			A base that containers can use to reuse shared code.
		*//******************************************************************/
		class ContainerBase
		{
		public:
			/*****************************************************************//*!
			\brief
				Constructor.
			\param title
				The title of the container.
			\param initialDimensions
				The dimensions of the window
			\param windowSizeCondFlags
				The flags of the setting of the window size
			*//******************************************************************/
			ContainerBase(const std::string& title, const Vec2& initialDimensions, FLAG_COND windowSizeCondFlags = FLAG_COND::NONE);

			/*****************************************************************//*!
			\brief
				Draws the container depending on whether the container is shown.
			*//******************************************************************/
			void Draw();

		protected:
			/*****************************************************************//*!
			\brief
				Called when the container is drawn. This function should call ImGui functions
				that set up and tear down the window.
			*//******************************************************************/
			virtual void DrawContainer() = 0;

			/*****************************************************************//*!
			\brief
				Called when the window is drawn. Custom implementations should override
				this and call various other gui namespace to populate the ImGui window
				with content.
			*//******************************************************************/
			virtual void DrawContents() = 0;

		protected:
			//! Whether this container is open
			bool isOpen;
			//! The title of this container
			std::string title;

		private:
			//! The initial size of this container
			Vec2 initialDimensions;
			//! The conditions that are passed to the ImGui function setting the container's dimensions.
			FLAG_COND windowSizeCondFlags;

		};

	}
}

namespace gui {

#pragma region Containers

	/*****************************************************************//*!
	\class Window
	\brief
		A construct not found in ImGui that wraps boilerplate code that draws an
		ImGui window. Derive this class and implement DrawContents() to create
		your own custom ImGui window.
	*//******************************************************************/
	class Window : public internal::ContainerBase
	{
	protected:
		/*****************************************************************//*!
		\brief
			Constructor.
		\param title
			The name of the window.
		\param initDimensions
			The initial width and height of the window.
		\param windowFlags
			The flags of the window.
		*//******************************************************************/
		Window(const std::string& title, const Vec2& initDimensions, FLAG_WINDOW windowFlags = FLAG_WINDOW::NONE);

		/*****************************************************************//*!
		\brief
			Draws this window. If overriding to modify some window style etc,
			remember to call base as otherwise nothing will draw!
		*//******************************************************************/
		virtual void DrawContainer() override;

	public:
		/*****************************************************************//*!
		\brief
			Gets whether this window is open.
		\return
			True if the window is open. False otherwise.
		*//******************************************************************/
		bool GetIsOpen() const;

		/*****************************************************************//*!
		\brief
			Sets whether this window is open.
		\param
			True if the window is open. False otherwise.
		*//******************************************************************/
		void SetIsOpen(bool newIsOpen);

	protected:
		/*****************************************************************//*!
		\brief
			Called when the state of isOpen changes. Implementations may hook onto
			this for special logic that executes when isOpen changes.
		*//******************************************************************/
		virtual void OnOpenStateChanged();

	protected:
		//! The window flags
		FLAG_WINDOW windowFlags;

	};

	/*****************************************************************//*!
	\class Child
	\brief
		Wraps ImGui::BeginChild() and ImGui::EndChild().
	*//******************************************************************/
	class Child : public internal::BeginEndBound_Child
	{
	public:
		//! ImGui::BeginChild()
		Child(const char* str_id, const Vec2& size = Vec2{}, FLAG_CHILD child_flags = FLAG_CHILD::NONE, FLAG_WINDOW window_flags = FLAG_WINDOW::NONE);
	};

	/*****************************************************************//*!
	\class Popup
	\brief
		Wraps ImGui::BeginPopup() and ImGui::EndPopup().
	*//******************************************************************/
	class Popup : public internal::BeginEndBound_Popup
	{
	public:
		//! ImGui::BeginPopup()
		Popup(const char* identifier, FLAG_WINDOW flags = FLAG_WINDOW::NONE);

		/*****************************************************************//*!
		\brief
			Sets a popup open state to open.
		\param identifier
			The popup identifier.
		*//******************************************************************/
		static void Open(const char* identifier);

		//! ImGui::CloseCurrentPopup()
		void Close();

		//! ImGui::IsWindowAppearing() - whether this window was opened this frame.
		bool WasOpenedThisFrame();
	};

	/*****************************************************************//*!
	\class PopupWindow
	\brief
		A construct not found in ImGui that wraps boilerplate code that draws an
		ImGui popup window. Derive this class and implement DrawContents() to create
		your own custom ImGui popup window.
	*//******************************************************************/
	class PopupWindow : public internal::ContainerBase
	{
	public:
		/*****************************************************************//*!
		\enum FLAG
		\brief
			Flags specific for popup.
		*//******************************************************************/
		enum class FLAG
		{
			NONE = 0,
			NO_CLOSE_BUTTON = 1,
		};

	protected:
		/*****************************************************************//*!
		\brief
			Constructor.
		\param title
			The name of the popup window.
		\param initDimensions
			The initial width and height of the popup window.
		*//******************************************************************/
		PopupWindow(const std::string& title, const Vec2& initDimensions);

		/*****************************************************************//*!
		\brief
			Draws this popup window. If overriding to modify some window style etc,
			remember to call base as otherwise nothing will draw!
		*//******************************************************************/
		virtual void DrawContainer() override;

		//! The flags of the popup.
		FLAG flags;
	};
	GENERATE_ENUM_CLASS_BITWISE_OPERATORS(PopupWindow::FLAG);

	/*****************************************************************//*!
	\class Tooltip
	\brief
		Wraps ImGui::BeginTooltip() and ImGui::EndTooltip(), or ImGui::SetTooltip().
	*//******************************************************************/
	class Tooltip : public internal::BeginEndBound_Tooltip
	{
	public:
		//! ImGui::BeginTooltip() and ImGui::EndTooltip().
		Tooltip();

		//! ImGui::SetTooltip()
		template <typename ...Args>
		Tooltip(const char* fmt, const Args&... args);
	};

	//! ImGui::GetWindowWidth
	float GetWindowWidth();
	//! ImGui::GetWindowHeight
	float GetWindowHeight();
	//! ImGui::GetWindowContentRegionMax()
	Vec2 GetWindowContentRegionMax();

	//! ImGui::SetNextWindowPos()
	void SetNextWindowPos(const Vec2& pos);
	//! ImGui::SetNextWindowSize()
	void SetNextWindowSize(const Vec2& size);
	//! ImGui::SetNextWindowSizeConstraints
	void SetNextWindowSizeConstraints(const Vec2& min, const Vec2& max);

	//! ImGui::GetFrameHeightWithSpacing()
	float GetFrameHeightWithSpacing();

	//! Non-ImGui function. Gets whether the current window is focused.
	bool IsCurrWindowFocused();

	//! ImGui::SetItemDefaultFocus()
	void SetItemDefaultFocus();
	//! ImGui::IsItemHovered()
	bool IsItemHovered();

#pragma endregion // Containers

#pragma region ID Management

	/*****************************************************************//*!
	\class SetID
	\brief
		Wraps ImGui::PushID() and ImGui::PopID(). Used to help ImGui differentiate
		items with the same labels.
	*//******************************************************************/
	class SetID
	{
	public:
		//! ImGui::PushID()
		SetID(int id);
		SetID(const char* label);

		//! ImGui::PopID()
		~SetID();
	};

#pragma endregion // ID Management

#pragma region Text

	//! ImGui::GetFontSize()
	float GetFontSize();

	/*****************************************************************//*!
	\class SetTextWrapPos
	\brief
		Wraps ImGui::PushTextWrapPos() and ImGui::PopTextWrapPos().
	*//******************************************************************/
	class SetTextWrapPos : public internal::BeginEndBound_TextWrapPos
	{
	public:
		//! ImGui::PushTextWrapPos()
		SetTextWrapPos(float wrap_local_pos_x = 0.0f);
	};

	//! ImGui::AlignTextToFramePadding()
	void AlignTextToFramePadding();

	//! ImGui::Text()
	template <typename ...Args>
	void TextFormatted(const char* fmt, const Args&... args);
	//! ImGui::TextUnformatted()
	void TextUnformatted(const char* text);
	void TextUnformatted(const std::string& text);
	void TextCenteredUnformatted(const char* text);
	void TextCenteredUnformatted(const std::string& text);
	//! ImGui::TextColored
	template <typename ...Args>
	void TextColored(const Vec4& color, const char* fmt, const Args&... args);
	//! ImGui::TextWrapped()
	template <typename ...Args>
	void TextWrapped(const char* fmt, const Args&... args);
	//! ImGui::TextDisabled()
	template <typename ...Args>
	void TextDisabled(const char* format, const Args&... args);

	//! ImGui::InputText() (displays read-only text within a text box)
	// TODO: See if can find a way to unify this and perhaps a base TextBox class.
	void TextBoxReadOnly(const char* label, const char* text, size_t size);
	void TextBoxReadOnly(const char* label, const std::string& text);

	/*****************************************************************//*!
	\class TextBoxWithBuffer
	\brief
		A construct not found in ImGui that wraps ImGui::InputText() and
		does storing of the input text string independently.
	*//******************************************************************/
	template <size_t BufferSize>
	class TextBoxWithBuffer
	{
	public:
		/*****************************************************************//*!
		\brief
			Constructor.
		\param label
			The label of the text field.
		*//******************************************************************/
		TextBoxWithBuffer(const char* label);

		/*****************************************************************//*!
		\brief
			Draws the text field.
		\param flags
			The flags of the input text field.
		\param callback
			The callback to call depending on the flags.
		*//******************************************************************/
		bool Draw(FLAG_INPUT_TEXT flags = FLAG_INPUT_TEXT::NONE, types::InputTextCallback callback = nullptr);

		/*****************************************************************//*!
		\brief
			Gets a read only pointer to the buffer.
		\return
			The buffer.
		*//******************************************************************/
		const char* GetBufferPtr() const;

		/*****************************************************************//*!
		\brief
			Gets the buffer as a string.
		\return
			The buffer.
		*//******************************************************************/
		std::string GetBuffer() const;

		/*****************************************************************//*!
		\brief
			Sets the buffer's contents to a specified string.
		\param text
			The content to set the buffer to.
		*//******************************************************************/
		void SetBuffer(const std::string& text);

		/*****************************************************************//*!
		\brief
			Clears the buffer.
		*//******************************************************************/
		void ClearBuffer();

	private:
		//! The input buffer
		char buffer[BufferSize];
		//! Label
		const char* label;
	};

	/*****************************************************************//*!
	\class TextBoxWithFilter
	\brief
		Wraps ImGuiTextFilter with a simplified interface. Requires a unique
		consistent instance per text field however (static, maybe we could find
		a better method idk).
	*//******************************************************************/
	class TextBoxWithFilter
#ifdef IMGUI_ENABLED
		: private ImGuiTextFilter
#endif
	{
	public:
		/*****************************************************************//*!
		\brief
			Constructor.
		\param matchStarting
			Whether candidates' starting characters must match the filter to pass,
			or candidates pass if any part of the candidate matches the filter.
		*//******************************************************************/
		TextBoxWithFilter(bool matchStarting = false);

		/*****************************************************************//*!
		\brief
			Draws a text field.
		\param label
			The label of the text field.
		\param width
			The width of the text field.
		*//******************************************************************/
		void Draw(const char* label, float width = 0.0f);

		/*****************************************************************//*!
		\brief
			Checks whether a string matches any filter that's currently typed into this
			text field by a user.
		\param text
			The text to test whether it passes the filter or not.
		*//******************************************************************/
		bool PassFilter(const char* text) const;

		/*****************************************************************//*!
		\brief
			Clears the text field.
		*//******************************************************************/
		void Clear();

		/*****************************************************************//*!
		\brief
			Checks if the text field is empty.
		*//******************************************************************/
		bool IsEmpty() const;

	private:
		bool matchStarting;
	};


#pragma endregion // Text

#pragma region Variables

	//! ImGui::Selectable
	bool Selectable(const char* label, bool isSelected = false);

	//! ImGui::Checkbox
	bool Checkbox(const char* label, bool* v);

	//! ImGui::DragInt, etc.
	bool VarDrag(const char* label, int* v, float speed = 1.0f, int min = 0, int max = 0);
	bool VarDrag(const char* label, float* v, float speed = 1.0f, float min = 0.0f, float max = 0.0f, const char* format = "%.2f");
	bool VarDrag(const char* label, Vector2* v, float speed = 1.0f, Vector2 min = {}, Vector2 max = {}, const char* format = "%.2f");

	//! ImGui::InputInt, etc.
	bool VarInput(const char* label, int* v, int step = 1);

	//! ImGui::SliderInt, etc.
	template <typename T>
	bool Slider(const char* label, T* v, const T& min, const T& max);

	/*****************************************************************//*!
	\brief
		Custom: Draw a vector with resize options.
	\tparam T
		The type of data stored within the vector.
	\tparam ElemDrawFuncType
		The function type to call to draw each element. Must be able to receive
		params [T].
	\param label
		The vector field label.
	\param v
		The vector.
	\param elemDrawFunc
		The function that draws each element within the vector.
	*//******************************************************************/
	template <typename T, typename ElemDrawFuncType>
	bool VarContainer(const char* label, std::vector<T>* v, ElemDrawFuncType elemDrawFunc);
	
	/* Storage */
	// Stores and gets variables in/from ImGui state storage.
	bool& GetVar(const char* label, bool defaultVal);

#pragma endregion // Variables

#pragma region Payload

	/*****************************************************************//*!
	\brief
		Wraps ImGui::BeginDragDropSource(), ImGui::SetDragDropPayload() and ImGui::EndDragDropSource().
	\tparam DataType
		The type of the payload data.
	\param identifier
		The identifier of the payload (to identify compatible payloads)
	\param data
		The payload data.
	\param dragLabel
		The name of the object being dragged.
	*//******************************************************************/
	template <typename DataType>
	void PayloadSource(const char* identifier, const DataType& data, const char* dragLabel = identifier);
	template <>
	void PayloadSource<std::string>(const char* identifier, const std::string& data, const char* dragLabel);

	/*****************************************************************//*!
	\brief
		Wraps ImGui::BeginDragDropTarget(), ImGui::AcceptDragDropPayload() and ImGui::EndDragDropTarget().
		Specifying the template parameter "DataType" is required for this function to know what to cast the payload data to.
	\tparam DataType
		The type of the expected payload data.
	\tparam FunctionType
		The type of the function to be called when receiving a payload.
	\param identifier
		The identifier of the payload (to identify compatible payloads)
	\param onReceive
		The function called when receiving a payload.
	*//******************************************************************/
	template <typename DataType, typename FunctionType>
		requires std::invocable<FunctionType, const DataType&>
	void PayloadTarget(const char* identifier, FunctionType onReceive);

#pragma endregion // Payload

#pragma region Button

	/*****************************************************************//*!
	\class Button
	\brief
		Wraps ImGui::Button()
	*//******************************************************************/
	class Button : public internal::BeginEndBound_Button
	{
	public:
		//! ImGui::Button()
		Button(const char* label, const Vec2& size = Vec2{});
	};

#pragma endregion // Button

#pragma region Combo

	/*****************************************************************//*!
	\class Combo
	\brief
		Wraps ImGui::BeginCombo() and ImGui::EndCombo(), or ImGui::Combo().
	*//******************************************************************/
	class Combo
	{
	public:
		//! ImGui::BeginCombo()
		Combo(const char* label, const char* previewValue, FLAG_COMBO flags = FLAG_COMBO::NONE);

		/*****************************************************************//*!
		\brief
			Calls ImGui::Combo() from a const char* array.
		\tparam ContType
			The type of the array.
		\param label
			The label of the combo.
		\param data
			The array.
		\param selectedValue
			The currently selected string.
		\param outSelectedIndex
			The index of the selected string will be written here.
		*//******************************************************************/
		template <typename ContType>
			requires util::ConvertibleToCArray<ContType> && std::is_same_v<typename ContType::value_type, const char*>
		Combo(const char* label, const ContType& data, const char* selectedValue, int* outSelectedIndex);

		/*****************************************************************//*!
		\brief
			Calls ImGui::Combo() from a const char* array.
		\tparam ContType
			The type of the array.
		\param label
			The label of the combo.
		\param data
			The array.
		\param selectedIndex
			The currently selected index within the array. If a selection
			happens, the new index will be written here.
		*//******************************************************************/
		template <typename ContType>
			requires util::ConvertibleToCArray<ContType> && std::is_same_v<typename ContType::value_type, const char*>
		Combo(const char* label, const ContType& data, int* selectedIndex);

		/*****************************************************************//*!
		\brief
			Calls ImGui::Combo() from a const char* array.
		\tparam ContType
			The type of the array.
		\param label
			The label of the combo.
		\param data
			The array.
		\param selectedIndex
			The currently selected index within the array. If a selection
			happens, the new index will be written here.
		*//******************************************************************/
		Combo(const char* label, const char* const* data, size_t numElems, int* selectedIndex);

		/*****************************************************************//*!
		\brief
			Destructor.
		*//******************************************************************/
		~Combo();

		/*****************************************************************//*!
		\brief
			Adds a selectable entry within this combo. Equivalent to ImGui::Selectable()
		\param label
			The entry's name.
		\param isSelected
			Whether this entry is currently selected.
		\return
			True if this entry was selected this frame update. False otherwise.
		*//******************************************************************/
		bool Selectable(const char* label, bool isSelected);

		/*****************************************************************//*!
		\brief
			NOTE: The behavior of this operator depends on which constructor was called!
			If constructed without elements, returns whether the combo window is open.
			If constructed with elements, returns whether any elements were selected.
		\return
			True or false depending on the conditions listed above.
		*//******************************************************************/
		operator bool() const;

	private:
		/*****************************************************************//*!
		\brief
			Calls ImGui::Combo().
		\param label
			The label of the combo.
		\param data
			Pointer to array of char strings that will be the data that populates the combo.
		\param numElems
			The number of elements within the array.
		\param selectedIndex
			Pointer to the selected index within the array. This will be overriden if a selection happened.
		\return
			True if a selection happened. False otherwise.
		*//******************************************************************/
		void CallCombo(const char* label, const char* const* data, size_t numElems, int* selectedIndex);

	private:
		//! Whether ImGui::EndCombo() should be called. This is set depending on which constructor was called.
		bool shouldCallEndCombo;
		//! Whether the user selected an option this frame update.
		bool selectionHappened{ false };
	};

#pragma endregion // Combo

#pragma region Menu

	/*****************************************************************//*!
	\class MenuBar
	\brief
		Wraps ImGui::BeginMenuBar() and ImGui::EndMenuBar().
	*//******************************************************************/
	class MenuBar : public internal::BeginEndBound_MenuBar
	{
	};

	/*****************************************************************//*!
	\class Menu
	\brief
		Wraps ImGui::BeginMenu() and ImGui::EndMenu().
	*//******************************************************************/
	class Menu : public internal::BeginEndBound_Menu
	{
	public:
		//! ImGui::BeginMenu()
		Menu(const char* label);
	};

	/*****************************************************************//*!
	\class ItemContextMenu
	\brief
		Wraps ImGui::BeginPopupContextItem() and ImGui::EndPopup().
		Used for dropdown selection menus when right clicking an item.
		Use MenuItem() to populate the list.
	*//******************************************************************/
	class ItemContextMenu : public internal::BeginEndBound_ItemContextMenu
	{
	public:
		//! ImGui::BeginPopupContextItem()
		ItemContextMenu(const char* label, FLAG_POPUP flags = FLAG_POPUP::MOUSE_RIGHT);
	};

	//! ImGui::MenuItem()
	bool MenuItem(const char* label, bool* p_selected = nullptr);

#pragma endregion // Menu

#pragma region Tree

	/*****************************************************************//*!
	\class CollapsingHeader
	\brief
		Wraps ImGui::CollapsingHeader(). Draws a header that is user-collapsable.
	*//******************************************************************/
	class CollapsingHeader : public internal::BeginEndBound_CollapsingHeader
	{
	public:
		//! ImGui::CollapsingHeader()
		CollapsingHeader(const char* label, FLAG_TREE_NODE flags = FLAG_TREE_NODE::NONE);
		CollapsingHeader(const std::string& label, FLAG_TREE_NODE flags = FLAG_TREE_NODE::NONE);
	};

#pragma endregion // Tree

#pragma region Table

	/*****************************************************************//*!
	\class Table
	\brief
		Wraps ImGui::BeginTable() and ImGui::EndTable().
	*//******************************************************************/
	class Table : public internal::BeginEndBound_Table
	{
	public:
		//! ImGui::BeginTable()
		Table(const char* label, int numCols, bool hasHeader, FLAG_TABLE flags = FLAG_TABLE::NONE, const Vec2& outerSize = Vec2{}, float innerWidth = 0.0f);

		//! ImGui::TableSetupColumn
		//! This is used to specify each column's properties and header label.
		void AddColumnHeader(const char* label, FLAG_TABLE_COLUMN flags = FLAG_TABLE_COLUMN::NONE, float width = 0.0f);
		//! ImGui::TableHeadersRow()
		//! Submits the column properties to the table. Must be called before populating the first column if hasHeader is true.
		void SubmitColumnHeaders();

		//! ImGui::TableNextColumn()
		void NextColumn();

	private:
		//! The flags submitted to the table. For custom flags in our implementation.
		FLAG_TABLE tableFlags;
	};

#pragma endregion // Table

#pragma region Alignment & Spacing

	/*****************************************************************//*!
	\class Group
	\brief
		Wraps ImGui::BeginGroup() and ImGui::EndGroup().
	*//******************************************************************/
	class Group : public internal::BeginEndBound_Group
	{
	};

	/*****************************************************************//*!
	\class Indent
	\brief
		Wraps ImGui::Indent() and ImGui::Unindent().
	*//******************************************************************/
	class Indent
	{
	public:
		/*****************************************************************//*!
		\brief
			Constructor. Wraps ImGui::Indent() and ImGui::Unindent().
		\param amt
			The amount to indent by.
			If <= 0.0f, uses default indent width.
		*//******************************************************************/
		Indent(float amt = 0.0f);

		/*****************************************************************//*!
		\brief
			Destructor.
		*//******************************************************************/
		~Indent();

	private:
		float indentAmt;
	};

	//! ImGui::Separator()
	void Separator();
	//! ImGui::Spacing()
	void Spacing();
	//! ImGui::SameLine()
	void SameLine(float offset_from_start_x = 0.0f, float spacing = -1.0f);

	//! ImGui::GetContentRegionAvail()
	Vec2 GetAvailableContentRegion();
	//! ImGui::GetItemRectSize()
	Vec2 GetPrevItemRectSize();
	//! ImGui::GetItemRectMin()
	Vec2 GetPrevItemRectMin();
	//! ImGui::GetItemRectMax()
	Vec2 GetPrevItemRectMax();

	//! ImGui::SetNextItemWidth()
	void SetNextItemWidth(float width);

#pragma endregion // Alignment & Spacing

#pragma region Styling

	/*****************************************************************//*!
	\class SetStyleColor
	\brief
		Wraps ImGui::PushStyleColor() and ImGui::PopStyleColor().
	*//******************************************************************/
	class SetStyleColor : public internal::BeginEndBound_StyleColor
	{
	public:
		/*****************************************************************//*!
		\brief
			Constructor. Calls ImGui::PushStyleColor().
		\param apply
			Whether to actually apply the style color. For flexibility in code flow.
		*//******************************************************************/
		SetStyleColor(FLAG_STYLE_COLOR idx, const Vec4& val, bool apply = true);
	};

	/*****************************************************************//*!
	\class UnsetStyleColor
	\brief
		Equivalent to calling ImGui::PopStyleColor() for the specified number of times.
		When this goes out of scope, effectively calls ImGui::PushStyleColor() pushing
		the popped style colors back onto the stack.
	*//******************************************************************/
	class UnsetStyleColor
	{
	public:
		/*****************************************************************//*!
		\brief
			Constructor.
			Calls ImGui::PopStyleColor() the specified number of times, saving the
			style color to a buffer.
		\param numPops
			The number of style colors to pop from the stack.
		*//******************************************************************/
		UnsetStyleColor(unsigned int numPops);

		/*****************************************************************//*!
		\brief
			Destructor.
			Calls ImGui::PushStyleColor() pushing the style colors within the buffer
			back onto the stack.
		*//******************************************************************/
		~UnsetStyleColor();

	private:
#ifdef IMGUI_ENABLED
		//! The buffer storing popped style colors.
		std::vector<ImGuiColorMod> popped;
#endif

	};

	/*****************************************************************//*!
	\class SetStyleVar
	\brief
		Wraps ImGui::PushStyleVar() and ImGui::PopStyleVar().
	*//******************************************************************/
	class SetStyleVar : public internal::BeginEndBound_StyleVar
	{
	public:
		//! ImGui::PushStyleVar()
		SetStyleVar(FLAG_STYLE_VAR idx, const Vec2& val);
	};

	/*****************************************************************//*!
	\class UnsetStyleVar
	\brief
		Equivalent to calling ImGui::PopStyleVar() for the specified number of times.
		When this goes out of scope, effectively calls ImGui::PushStyleVar() pushing
		the popped style vars back onto the stack.
	*//******************************************************************/
	class UnsetStyleVar
	{
	public:
		/*****************************************************************//*!
		\brief
			Constructor.
			Calls ImGui::PopStyleVar() the specified number of times, saving the
			style var to a buffer.
		\param numPops
			The number of style vars to pop from the stack.
		*//******************************************************************/
		UnsetStyleVar(unsigned int numPops);

		/*****************************************************************//*!
		\brief
			Destructor.
			Calls ImGui::PushStyleVar() pushing the style vars within the buffer
			back onto the stack.
		*//******************************************************************/
		~UnsetStyleVar();

	private:
#ifdef IMGUI_ENABLED
		//! The buffer storing popped style vars.
		std::vector<ImGuiStyleMod> popped;
#endif

	};

	//! ImGui::GetStyleColorVec4()
	Vec4 GetStyleColor(FLAG_STYLE_COLOR style);

#pragma endregion // Styling

#pragma region Custom Drawables

	//! ImGui::GetWindowDrawList()->AddTriangleFilled()
	void DrawTriangle(Vec2 p0, Vec2 p1, Vec2 p2, const Vec4& color);

#pragma endregion // Custom Drawables

#pragma region Misc

	class Disabled : public internal::BeginEndBound_Disabled
	{
	public:
		//! ImGui::BeginDisabled()
		Disabled(bool isDisabled = true);
	};

	//! ImGui::IsKeyPressed()
	bool IsKeyPressed(KEY key, bool repeating = true);
	//! ImGui::SetKeyboardFocusHere()
	void SetKeyboardFocusHere(int offset = 0);

	//! ImGui::SetScrollHereY()
	void SetScrollHereY(float center_y_ratio = 0.5f);

#pragma endregion // Misc

}

#include "GUICollection.ipp"
