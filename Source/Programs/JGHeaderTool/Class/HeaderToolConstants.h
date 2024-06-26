#pragma once
#include "Core.h"

namespace HHeaderToolConstants
{
	namespace Token
	{
		static const PString Class = "JGCLASS";
		static const PString Enum  = "JGENUM";
		static const PString EnumMeta = "JGENUMMETA";
		static const PString Property = "JGPROPERTY";
		static const PString Function = "JGFUNCTION";

		static const PString ModuleName = "{MODULE_NAME}";
		static const PString FileName = "{FILE_NAME}";
		static const PString ClassName = "{CLASS_NAME}";
		static const PString EnumName = "{ENUM_NAME}";

		static const PString CodeGenerationIncludeBegin = "CODE_GENERATION_INCLUDE_BEGIN()";
		static const PString CodeGenerationIncludeEnd = "CODE_GENERATION_INCLUDE_END()";

		static const PString CodeGenerationBegin = "CODE_GENERATION_BEGIN()";
		static const PString CodeGenerationEnd = "CODE_GENERATION_END()";
	}

	namespace Template
	{
		static const PString CodeGenHeaderFileName = "{FILE_NAME}.generation.h";
		static const PString CodeGenCppFileName = "{FILE_NAME}.generation.cpp";

		static const PString CodeGenCreateStaticClassFunction = "PSharedPtr<JGClass> Module_{MODULE_NAME}_Code_Generation_Static_Create_Class_{CLASS_NAME}()";
		static const PString CodeGenCreateClassFunction  = "PSharedPtr<JGClass> Module_{MODULE_NAME}_Code_Generation_Create_Class_{CLASS_NAME}(const JGObject* fromThis)";
		static const PString CodeGenCreateObjectFunction = "PSharedPtr<JGObject> Module_{MODULE_NAME}_Code_Generation_Create_Object_{CLASS_NAME}()";


		static const PString CodeGenCreateStaticEnumFunction = "PSharedPtr<JGEnum> Module_{MODULE_NAME}_Code_Generation_Static_Create_Enum_{ENUM_NAME}()";
		static const PString CodeGenCreateEnumFunction = "PSharedPtr<JGEnum> Module_{MODULE_NAME}_Code_Generation_Create_Enum_{ENUM_NAME}(const {ENUM_NAME}* fromThis)";


	}


};