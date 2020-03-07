#include "AmazonS3Utils.h"

ListObjectsResult AmazonS3Utils::GetListObjects(std::string accessKeyId, std::string secretAccessKey, std::string bucketName)
{
	std::string bucketRegion;

	//Obtain bucket region
	try
	{
		{
			CAmazonS3Client client(accessKeyId, secretAccessKey);

			GetBucketLocationRequest request;
			request.bucket = bucketName;

			auto result = client.GetBucketLocation(request);
			bucketRegion = result.locationConstraint;
		}

		//List objects
		CAmazonS3Client client(accessKeyId, secretAccessKey, bucketRegion);
		return client.ListObjects(bucketName);
	}
	catch(...)
	{
		return ListObjectsResult();
	}
}