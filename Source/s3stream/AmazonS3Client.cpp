#include "http/HttpClientFactory.h"
#include <map>
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include "HashUtils.h"
#include "string_format.h"
#include "xml/Parser.h"
#include "AmazonS3Client.h"

#define S3_HOSTNAME "s3.amazonaws.com"

static std::string hashToString(const std::array<uint8, 0x20> hash)
{
	std::string result;
	for(uint32 i = 0; i < 0x20; i += 8)
	{
		result += string_format("%02x%02x%02x%02x%02x%02x%02x%02x",
		    hash[i + 0], hash[i + 1], hash[i + 2], hash[i + 3],
		    hash[i + 4], hash[i + 5], hash[i + 6], hash[i + 7]
		);
	}
	return result;
}

std::string buildCanonicalRequest(Framework::Http::HTTP_VERB method, const std::string& uri, const std::string& query, const std::string& hashedPayload, const Framework::Http::HeaderMap& headers)
{
	std::string result;
	switch(method)
	{
	case Framework::Http::HTTP_VERB::DELETE:
		result += "DELETE";
		break;
	case Framework::Http::HTTP_VERB::HEAD:
		result += "HEAD";
		break;
	case Framework::Http::HTTP_VERB::GET:
		result += "GET";
		break;
	case Framework::Http::HTTP_VERB::PUT:
		result += "PUT";
		break;
	default:
		assert(false);
		break;
	}
	result += "\n";

	//CanonicalURI
	result += uri;
	result += "\n";

	//CanonicalQueryString
	result += query;
	result += "\n";

	//CanonicalHeaders
	for(const auto& headerPair : headers)
	{
		auto headerKey = headerPair.first;
		auto headerValue = headerPair.second;
		std::transform(headerKey.begin(), headerKey.end(), headerKey.begin(), &::tolower);
		result += headerKey;
		result += ":";
		result += headerValue;
		result += "\n";
	}
	result += "\n";

	//SignedHeaders
	for(auto headerPairIterator	= headers.begin();
	    headerPairIterator != headers.end(); headerPairIterator++)
	{
		bool isEnd =
		    [&] ()
		    {
			    auto headerPairIteratorCopy = headerPairIterator;
				headerPairIteratorCopy++;
				return headerPairIteratorCopy == std::end(headers);
		    }();
		auto headerKey = headerPairIterator->first;
		std::transform(headerKey.begin(), headerKey.end(), headerKey.begin(), &::tolower);
		result += headerKey;
		if(!isEnd)
		{
			result += ";";
		}
	}
	result += "\n";

	//HashedPayload
	result += hashedPayload;

	return result;
}

std::string buildStringToSign(const std::string& timestamp, const std::string& scope, const std::string& canonicalRequest)
{
	auto result = std::string("AWS4-HMAC-SHA256\n");
	result += timestamp;
	result += "\n";

	result += scope;
	result += "\n";

	auto canonicalRequestHash = hashToString(Framework::HashUtils::ComputeSha256(canonicalRequest.data(), canonicalRequest.size()));
	result += canonicalRequestHash;

	return result;
}

std::array<uint8, 0x20> buildSigningKey(const std::string& secretAccessKey, const std::string& date, const std::string& region, const std::string& service, const std::string& requestType)
{
	auto signKey = "AWS4" + secretAccessKey;

	auto result = Framework::HashUtils::ComputeHmacSha256(signKey.c_str(), signKey.length(), date.c_str(), date.length());
	result = Framework::HashUtils::ComputeHmacSha256(result.data(), result.size(), region.c_str(), region.length());
	result = Framework::HashUtils::ComputeHmacSha256(result.data(), result.size(), service.c_str(), service.length());
	result = Framework::HashUtils::ComputeHmacSha256(result.data(), result.size(), requestType.c_str(), requestType.length());
	return result;
}

std::string timeToString(const tm* timeInfo)
{
	static const size_t bufferSize = 0x100;
	char output[bufferSize];
	auto result = strftime(output, bufferSize, "%Y%m%dT%H%M%SZ", timeInfo);
	if(result == 0)
	{
		throw std::runtime_error("Failed to convert time");
	}
	return std::string(output);
}

CAmazonS3Client::CAmazonS3Client(std::string accessKeyId, std::string secretAccessKey, std::string region)
    : m_accessKeyId(std::move(accessKeyId))
    , m_secretAccessKey(std::move(secretAccessKey))
    , m_region(std::move(region))
{

}

GetBucketLocationResult CAmazonS3Client::GetBucketLocation(const GetBucketLocationRequest& request)
{
	Request rq;
	rq.method  = Framework::Http::HTTP_VERB::GET;
	rq.host    = string_format("%s." S3_HOSTNAME, request.bucket.c_str());
	rq.urlHost = S3_HOSTNAME;
	rq.uri     = "/";
	rq.query   = "location=";
	
	auto response = ExecuteRequest(rq);
	if(response.statusCode != Framework::Http::HTTP_STATUS_CODE::OK)
	{
		throw std::runtime_error("Failed to get bucket location.");
	}

	GetBucketLocationResult result;

	auto documentNode = std::unique_ptr<Framework::Xml::CNode>(Framework::Xml::CParser::ParseDocument(response.data));
	auto locationConstraintNode = documentNode->Select("LocationConstraint");
	if(locationConstraintNode)
	{
		auto locationConstraint = locationConstraintNode->GetInnerText();
		result.locationConstraint = locationConstraint ? locationConstraint : "";
	}

	return result;
}

GetObjectResult CAmazonS3Client::GetObject(const GetObjectRequest& request)
{
	Request rq;
	rq.method  = Framework::Http::HTTP_VERB::GET;
	rq.uri     = "/" + Framework::Http::CHttpClient::UrlEncode(request.object);
	rq.host    = string_format("%s." S3_HOSTNAME, request.bucket.c_str());
	rq.urlHost = rq.host;

	if(request.range.first != request.range.second)
	{
		auto rangeHeader = string_format("bytes=%llu-%llu", request.range.first, request.range.second);
		rq.headers.insert(std::make_pair("Range", rangeHeader));
	}

	auto response = ExecuteRequest(rq);
	if(
	    (response.statusCode != Framework::Http::HTTP_STATUS_CODE::OK) &&
	    (response.statusCode != Framework::Http::HTTP_STATUS_CODE::PARTIAL_CONTENT)
	)
	{
		throw std::runtime_error("Failed to get object.");
	}

	GetObjectResult result;
	uint32 dataLength = response.data.GetLength();
	result.data.resize(dataLength);
	response.data.Read(result.data.data(), dataLength);
	return result;
}

HeadObjectResult CAmazonS3Client::HeadObject(const HeadObjectRequest& request)
{
	Request rq;
	rq.method  = Framework::Http::HTTP_VERB::HEAD;
	rq.uri     = "/" + Framework::Http::CHttpClient::UrlEncode(request.object);
	rq.host    = string_format("%s." S3_HOSTNAME, request.bucket.c_str());
	rq.urlHost = rq.host;

	auto response = ExecuteRequest(rq);
	if(response.statusCode != Framework::Http::HTTP_STATUS_CODE::OK)
	{
		throw std::runtime_error("Failed to head object.");
	}

	HeadObjectResult result;

	auto contentLengthIterator = response.headers.find("Content-Length");
	if(contentLengthIterator != std::end(response.headers))
	{
		result.contentLength = atoll(contentLengthIterator->second.c_str());
	}

	auto etagIterator = response.headers.find("ETag");
	if(etagIterator != std::end(response.headers))
	{
		result.etag = etagIterator->second;
	}

	return result;
}

ListObjectsResult CAmazonS3Client::ListObjects(std::string bucket)
{
	Request rq;
	rq.method  = Framework::Http::HTTP_VERB::GET;
	rq.uri     = "/";
	rq.host    = string_format("%s." S3_HOSTNAME, bucket.c_str());
	rq.urlHost = rq.host;

	auto response = ExecuteRequest(rq);
	if(response.statusCode != Framework::Http::HTTP_STATUS_CODE::OK)
	{
		throw std::runtime_error("Failed to list objects");
	}

	ListObjectsResult result;

	auto documentNode = std::unique_ptr<Framework::Xml::CNode>(Framework::Xml::CParser::ParseDocument(response.data));
	auto contentsNodes = documentNode->SelectNodes("ListBucketResult/Contents");
	for(const auto& contentsNode : contentsNodes)
	{
		Object object;
		if(auto keyNode = contentsNode->Select("Key"))
		{
			object.key = keyNode->GetInnerText();
		}
		result.objects.push_back(object);
	}

	return result;
}

Framework::Http::RequestResult CAmazonS3Client::ExecuteRequest(const Request& request)
{
	assert(!m_accessKeyId.empty());
	assert(!m_secretAccessKey.empty());
	assert(!request.host.empty());
	assert(!request.urlHost.empty());

	time_t rawTime;
	time(&rawTime);
	auto timeInfo = gmtime(&rawTime);

	auto date = string_format("%04d%02d%02d", timeInfo->tm_year + 1900, timeInfo->tm_mon + 1, timeInfo->tm_mday);
	auto service = std::string("s3");
	auto requestType = std::string("aws4_request");

	auto content = std::string();
	auto contentHashString = hashToString(Framework::HashUtils::ComputeSha256(content.data(), content.size()));

	auto scope = string_format("%s/%s/%s/%s", date.c_str(), m_region.c_str(), service.c_str(), requestType.c_str());
	auto timestamp = timeToString(timeInfo);

	Framework::Http::HeaderMap headers;
	headers.insert(std::make_pair("Host", request.host));
	headers.insert(std::make_pair("x-amz-content-sha256", contentHashString));
	headers.insert(std::make_pair("x-amz-date", timestamp));

	auto canonicalRequest = buildCanonicalRequest(request.method, request.uri, request.query, contentHashString, headers);
#ifdef DEBUG_REQUEST
	printf("canonicalRequest:\n%s\n\n", canonicalRequest.c_str());
#endif

	auto stringToSign = buildStringToSign(timestamp, scope, canonicalRequest);
#ifdef DEBUG_REQUEST
	printf("stringToSign:\n%s\n\n", stringToSign.c_str());
#endif

	auto signingKey = buildSigningKey(m_secretAccessKey, date, m_region, service, requestType);
	auto signature = hashToString(Framework::HashUtils::ComputeHmacSha256(signingKey.data(), signingKey.size(), stringToSign.c_str(), stringToSign.length()));

	auto authorizationString = string_format("AWS4-HMAC-SHA256 Credential=%s/%s, SignedHeaders=host;x-amz-content-sha256;x-amz-date, Signature=%s",
	    m_accessKeyId.c_str(), scope.c_str(), signature.c_str());
	headers.insert(std::make_pair("Authorization", authorizationString));
	headers.insert(request.headers.begin(), request.headers.end());

	auto url = string_format("https://%s%s", request.urlHost.c_str(), request.uri.c_str());
	if(!request.query.empty())
	{
		url += "?";
		url += request.query;
	}

	auto httpClient = Framework::Http::CreateHttpClient();
	httpClient->SetVerb(request.method);
	httpClient->SetUrl(url);
	httpClient->SetHeaders(headers);
	//httpClient->SetRequestBody(content);
	auto response = httpClient->SendRequest();
#ifdef DEBUG_REQUEST
	auto responseString = std::string(response.data.GetBuffer(), response.data.GetBuffer() + response.data.GetLength());
	printf("%s", responseString.c_str());
#endif
	return response;
}
