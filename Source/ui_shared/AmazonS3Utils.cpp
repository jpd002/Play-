#include "AmazonS3Utils.h"

ListObjectsResult AmazonS3Utils::GetListObjects(const CAmazonConfigs& credentials, std::string bucketName)
{
	std::string bucketRegion;

	//Obtain bucket region
	try
	{
		{
			CAmazonS3Client client(credentials);

			GetBucketLocationRequest request;
			request.bucket = bucketName;

			auto result = client.GetBucketLocation(request);
			bucketRegion = result.locationConstraint;
		}

		//List objects
		CAmazonS3Client client(credentials, bucketRegion);
		return client.ListObjects(bucketName);
	}
	catch(...)
	{
		return ListObjectsResult();
	}
}
