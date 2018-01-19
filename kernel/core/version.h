#ifndef VERSION_H
#define VERSION_H

namespace AutoVersion{
	
	//Date Version Types
	static const char DATE[] = "25";
	static const char MONTH[] = "12";
	static const char YEAR[] = "2017";
	static const char UBUNTU_VERSION_STYLE[] =  "17.12";
	
	//Software Status
	static const char STATUS[] =  "Coulomb";
	static const char STATUS_SHORT[] =  "c";
	
	//Standard Version Type
	static const long MAJOR  = 1;
	static const long MINOR  = 1;
	static const long BUILD  = 0;
	static const long REVISION  = 243;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 3796;
	#define RC_FILEVERSION 1,1,0,243
	#define RC_FILEVERSION_STRING "1, 1, 0, 243\0"
	static const char FULLVERSION_STRING [] = "1.1.0.243";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 0;
	

}
#endif //VERSION_H
