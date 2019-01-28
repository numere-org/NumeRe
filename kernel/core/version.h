#ifndef VERSION_H
#define VERSION_H

namespace AutoVersion{
	
	//Date Version Types
	static const char DATE[] = "28";
	static const char MONTH[] = "01";
	static const char YEAR[] = "2019";
	static const char UBUNTU_VERSION_STYLE[] =  "19.01";
	
	//Software Status
	static const char STATUS[] =  "Carnot";
	static const char STATUS_SHORT[] =  "C";
	
	//Standard Version Type
	static const long MAJOR  = 1;
	static const long MINOR  = 1;
	static const long BUILD  = 1;
	static const long REVISION  = 464;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 3930;
	#define RC_FILEVERSION 1,1,1,464
	#define RC_FILEVERSION_STRING "1, 1, 1, 464\0"
	static const char FULLVERSION_STRING [] = "1.1.1.464";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 0;
	

}
#endif //VERSION_H
