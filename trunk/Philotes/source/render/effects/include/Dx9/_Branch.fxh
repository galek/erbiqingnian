//
// _Branch - header file for static and uniform shader branch definitions and macros
//

//--------------------------------------------------------------------------------------------
// DEFINES
//--------------------------------------------------------------------------------------------

#define _OFF		0
#define _ON			1
#define _ON_VS		2
#define _ON_PS		3


//--------------------------------------------------------------------------------------------
// MACROS
//--------------------------------------------------------------------------------------------

#define _BRANCH_PARAM_NAME		add_SHADER_BRANCH_TYPE_to_args_list
#define SHADER_BRANCH_TYPE		uniform branch _BRANCH_PARAM_NAME
#define B(brnch)				( _BRANCH_PARAM_NAME == brnch || brnch == _ON )


// If you get an error here, you have used an invalid combination of branch flags!

#define ASSERT_BRANCH_COMBINATION(expr)			if ( ! (expr) ) { half _a[1]; _a[-1]; }
#define ASSERT_NOT_BRANCH_COMBINATION(expr)		ASSERT_BRANCH_COMBINATION( !(expr) )


//--------------------------------------------------------------------------------------------
// TYPEDEFS
//--------------------------------------------------------------------------------------------

typedef int branch;
