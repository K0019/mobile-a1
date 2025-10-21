// We are forced to use Clang when compiling for android, and clang has A LOT of very dumb restrictions.
// This file exists to modify our code via the preprocessor to save us a lot of time and pain from modifying each instance within our code just to conform to clang's stupid rules.

#ifndef ANDROID_MACROS_DISABLE_ADDING_TEMPLATE
// Calling a template function with explicit template params causes clang to complain about a missing 'template' keyword,
// even though it's very clear that we're doing a template function call...
#define GetComp template GetComp
#define RemoveComp template RemoveComp
#define RemoveCompNow template RemoveCompNow

#else
// Certain files aren't compatible with these macros (e.g. they need to define the GetComp function). Undef these defines for them.
#undef GetComp
#undef RemoveComp
#undef RemoveCompNow

#endif
