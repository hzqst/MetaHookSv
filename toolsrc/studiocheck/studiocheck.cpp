#include <iostream>
#include <IUtilAssetsIntegrity.h>

void UtilAssetsIntegrity_Init();
void UtilAssetsIntegrity_Shutdown();

int main(int argc, const char **argv)
{
	if (argc < 2)
	{
		std::cerr << "[Error] No arg(s) available.\n";
		return 1;
	}

	FILE *fp = fopen(argv[1], "rb");
	if (!fp)
	{
		std::cerr << "[Error] Failed to read file " << argv[1] << "\n";
		return 1;
	}

	fseek(fp, 0, SEEK_END);
	auto bufSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	auto buf = (unsigned char *)malloc(bufSize);
	if (!buf)
	{
		std::cerr << "[Error] Failed to allocate memory for " << argv[1] << "\n";
		fclose(fp);
		return 1;
	}

	UtilAssetsIntegrity_Init();

	fread(buf, bufSize, 1, fp);
	fclose(fp);

	UtilAssetsIntegrityCheckResult_StudioModel checkResult;
	auto reason = UtilAssetsIntegrity()->CheckStudioModel(buf, bufSize, &checkResult);

	UtilAssetsIntegrity_Shutdown();

	if (reason != UtilAssetsIntegrityCheckReason::OK)
	{
		std::cerr << "[Error] Integrity check failed for " << argv[1] << "\n";
		std::cerr << "[Reason] " << checkResult.ReasonStr << "\n";
		return 1;
	}

	free(buf);
	std::cerr << "[Info] Integrity check succeeded for " << argv[1] << "\n";
	return 0;
}