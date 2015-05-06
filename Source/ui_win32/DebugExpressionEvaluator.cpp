#include "DebugExpressionEvaluator.h"
#include <ctype.h>
#include <assert.h>

uint32 CDebugExpressionEvaluator::Evaluate(const char* expression, CMIPS* context)
{
	TokenArray tokens = Parse(expression);
	uint32 result = Evaluate(tokens, context);
	return result;
}

CDebugExpressionEvaluator::TokenArray CDebugExpressionEvaluator::Parse(const char* expression)
{
	TokenArray tokens;

	TOKEN currentToken;
	currentToken.type = TOKEN_TYPE_INVALID;

	while(1)
	{
		char currChar = (*expression++);
		if(currChar == 0)
		{
			if(currentToken.type != TOKEN_TYPE_INVALID)
			{
				tokens.push_back(currentToken);
				currentToken.type = TOKEN_TYPE_INVALID;
			}
			break;
		}
		else if(currChar == ' ' || currChar == '\t')
		{
			if(currentToken.type != TOKEN_TYPE_INVALID)
			{
				tokens.push_back(currentToken);
				currentToken.type = TOKEN_TYPE_INVALID;
			}
		}
		else if(currChar == '+' || currChar == '-' || currChar == '*' || currChar == '/')
		{
			if(currentToken.type != TOKEN_TYPE_INVALID)
			{
				tokens.push_back(currentToken);
				currentToken.type = TOKEN_TYPE_INVALID;
			}
			
			currentToken.type = TOKEN_TYPE_OP;
			currentToken.value = currChar;
			tokens.push_back(currentToken);

			currentToken.type = TOKEN_TYPE_INVALID;
		}
		else if(currentToken.type == TOKEN_TYPE_INVALID)
		{
			if(isdigit(currChar))
			{
				currentToken.type = TOKEN_TYPE_NUMBER;
				currentToken.value = currChar;
			}
			else
			{
				currentToken.type = TOKEN_TYPE_SYMBOL;
				currentToken.value = currChar;
			}
		}
		else if(currentToken.type != TOKEN_TYPE_INVALID)
		{
			currentToken.value += currChar;
		}
		else
		{
			assert(0);
		}
	}

	return tokens;
}

uint32 CDebugExpressionEvaluator::Evaluate(const TokenArray& tokens, CMIPS* context)
{
	OP_TYPE opType = OP_FIRST;

	uint32 result = 0;
	uint32 operand = 0;

	for(TokenArray::const_iterator tokenIterator(tokens.begin());
		tokens.end() != tokenIterator; tokenIterator++)
	{
		const TOKEN& token(*tokenIterator);
		switch(token.type)
		{
		case TOKEN_TYPE_NUMBER:
			{
				uint32 value = 0;
				if(token.value.find("0x") != std::string::npos)
				{
					if(sscanf(token.value.c_str(), "%x", &value) != 1)
					{
						throw std::runtime_error("Couldn't parse number.");
					}
				}
				else
				{
					if(sscanf(token.value.c_str(), "%d", &value) != 1)
					{
						throw std::runtime_error("Couldn't parse number.");
					}
				}
				operand = value;
			}
			break;
		case TOKEN_TYPE_SYMBOL:
			{
				uint32 value = 0;
				bool found = false;
				for(unsigned int i = 0; i < 32; i++)
				{
					if(!token.value.compare(CMIPS::m_sGPRName[i]))
					{
						value = context->m_State.nGPR[i].nV0;
						found = true;
					}
				}
				if(!found)
				{
					std::string message = std::string("Unknown symbol '");
					message += token.value;
					message += std::string("'.");
					throw std::runtime_error(message);
				}
				operand = value;
			}
			break;
		case TOKEN_TYPE_OP:
			{
				if(opType != OP_INVALID)
				{
					throw std::runtime_error("Invalid expression.");
				}
				if(!token.value.compare("+"))
				{
					opType = OP_ADD;
				}
				else if(!token.value.compare("-"))
				{
					opType = OP_SUB;
				}
				else if(!token.value.compare("*"))
				{
					opType = OP_MUL;
				}
				else if(!token.value.compare("/"))
				{
					opType = OP_DIV;
				}
				else
				{
					throw std::runtime_error("Unknown operator type.");
				}
			}
			break;
		default:
			assert(0);
			break;
		}

		if(token.type != TOKEN_TYPE_OP)
		{
			switch(opType)
			{
			case OP_FIRST:
				result = operand;
				break;
			case OP_ADD:
				result += operand;
				break;
			case OP_SUB:
				result -= operand;
				break;
			case OP_MUL:
				result *= operand;
				break;
			case OP_DIV:
				result /= operand;
				break;
			case OP_INVALID:
				throw std::runtime_error("Invalid expression (missing operator).");
				break;
			default:
				assert(0);
				break;
			}

			opType = OP_INVALID;
		}
	}

	return result;
}
