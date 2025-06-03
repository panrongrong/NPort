1. sd、emmc 有1.8 / 3.3v使用区别; 而且，sd和emmc有条件判断区别
2. sd、emmc dosfs需要组件：
	#define DRV_MMCSTORAGE_CARD
	#define DRV_SDSTORAGE_CARD
	#define INCLUDE_SD_BUS
	其他就是dosfs相关组件。
	
	
