extern "C"
{
	void cudaInit();

	void allocateArray(void **devPtr, int size);
	void freeArray(void *devPtr);
	void threadSync();
	void copyArrayFromDevice(void* host, const void* device, int size);
	void copyArrayToDevice(void* device, const void* host, int offset, int size);
	void setParameters(AOParams *hostParams);
	
}