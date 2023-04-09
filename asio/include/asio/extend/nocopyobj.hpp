//
// nocopyobj.hpp
// add by [01/06/2023 cuihaoyun]
//

#ifndef __NoCopyObj_HPP__
#define __NoCopyObj_HPP__
namespace asio {

	class NoCopyObj {
	public:
		NoCopyObj() {}
		~NoCopyObj() {}
	private:
		NoCopyObj(const NoCopyObj&) = delete;
		const NoCopyObj& operator =(const NoCopyObj&) = delete;
	};

}

#endif // __NoCopyObj_HPP__
