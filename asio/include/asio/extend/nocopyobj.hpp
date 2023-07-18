//
// nocopyobj.hpp
// add by [01/06/2023 cuihaoyun]
//

#ifndef __NoCopyObj_HPP__
#define __NoCopyObj_HPP__
namespace asio {

	//! Base class for types that should not be assigned.
	class NoAssign {
	public:
		NoAssign& operator=(const NoAssign&) = delete;
		NoAssign(const NoAssign&) = default;
		NoAssign() = default;
	};

	//! Base class for types that should not be copied or assigned.
	class NoCopyObj : NoAssign {
	public:
		NoCopyObj(const NoCopyObj&) = delete;
		NoCopyObj() = default;
	};


}

#endif // __NoCopyObj_HPP__
