/*
 * GameTask.h
 *
 *  Created on: Aug 28, 2014
 *      Author: rlcevg
 */

#ifndef GAMETASK_H_
#define GAMETASK_H_

#include <memory>

namespace circuit {

class CGameTask {
public:
	template<typename _Callable, typename... _Args>
		explicit CGameTask(_Callable&& __f, _Args&&... __args) {
			__b = _M_make_routine(std::__bind_simple(std::forward<_Callable>(__f), std::forward<_Args>(__args)...));
		}
	virtual ~CGameTask();

	void Run();

private:
    struct _Impl_base;
    typedef std::shared_ptr<_Impl_base>	__shared_base_type;
	struct _Impl_base {
		__shared_base_type	_M_this_ptr;
		inline virtual ~_Impl_base();
		virtual void _M_run() = 0;
	};
	template<typename _Callable> struct _Impl : public _Impl_base {
		_Callable _M_func;
		_Impl(_Callable&& __f) : _M_func(std::forward<_Callable>(__f)) {}
		void _M_run() { _M_func(); }
	};

	template<typename _Callable> std::shared_ptr<_Impl<_Callable>> _M_make_routine(_Callable&& __f) {
		// Create and allocate full data structure, not base.
		return std::make_shared<_Impl<_Callable>>(std::forward<_Callable>(__f));
	}

	__shared_base_type __b;

public:
	static std::shared_ptr<CGameTask> EmptyTask;
};

inline CGameTask::_Impl_base::~_Impl_base() = default;

} // namespace circuit

#endif // GAMETASK_H_
