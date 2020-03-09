#pragma once

#include <string>
#include "../s3stream/AmazonS3Client.h"

namespace AmazonS3Utils
{
	ListObjectsResult GetListObjects(std::string, std::string, std::string);
};
