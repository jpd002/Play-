#pragma once

#include <vector>
#include <future>

//This is meant to be used to execute callbacks when futures become ready
//Both the register and process functions must be called from the same thread
class CFutureContinuationManager
{
public:
	template <typename ResultType, typename CallbackType>
	void Register(std::future<ResultType> future, CallbackType callback)
	{
		auto futureWrapper = std::make_unique<CFutureWrapper<ResultType>>(std::move(future), std::move(callback));
		m_futures.push_back(std::move(futureWrapper));
	}

	void Execute()
	{
		auto newEnd = std::remove_if(m_futures.begin(), m_futures.end(), 
			[] (const FuturePtr& future) { return future->IsDone(); } );
		m_futures.erase(newEnd, m_futures.end());
	}

private:
	class CFuture
	{
	public:
		virtual ~CFuture() = default;
		virtual bool IsDone() = 0;
	};
	typedef std::unique_ptr<CFuture> FuturePtr;

	template<typename ResultType>
	class CFutureWrapper : public CFuture
	{
	public:
		typedef std::function<void (const ResultType&)> CallbackType;

		CFutureWrapper(std::future<ResultType> future, CallbackType callback)
			: m_future(std::move(future)), m_callback(std::move(callback))
		{
			
		}

		bool IsDone() override
		{
			if(!m_future.valid()) return false;
			auto status = m_future.wait_for(std::chrono::seconds(0));
			if(status != std::future_status::ready) return false;

			auto result = m_future.get();
			m_callback(result);
			return true;
		}

	private:
		std::future<ResultType> m_future;
		CallbackType m_callback;
	};

	std::vector<FuturePtr> m_futures;
};
