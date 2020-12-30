#pragma once

#define PP_CAT(a, b) PP_CAT_I(a, b)
#define PP_CAT_I(a, b) PP_CAT_II(~, a ## b)
#define PP_CAT_II(p, res) res

#define UNIQUE_NAME(base) PP_CAT(base, __LINE__)

#define SCOPE_EXIT ScopeExit UNIQUE_NAME(tmp) = [&]

class ScopeExit
{
public:
    template<class Callback_t>
    ScopeExit(Callback_t&& callback)
		: m_caller(new caller<Callback_t>(std::forward<Callback_t>(callback)))
	    {}
	~ScopeExit()
    {
        m_caller->call();
		delete m_caller;
    }
private:
	struct caller_interface
	{
		virtual ~caller_interface() = default;
		virtual void call() = 0;
	};

    template<class Callback_t>
	struct caller: caller_interface
	{
		explicit caller(Callback_t&& callback )
            : m_callback(std::forward<Callback_t>(callback))
            {}
		void call() override
		{
		    m_callback();
		}
		Callback_t m_callback;
	};
	caller_interface* m_caller;
};