//
// object.hpp
// add by [01/06/2023 cuihaoyun]
//

#ifndef __NoCopyObj_HPP__
#define __NoCopyObj_HPP__
namespace asio {

	class NoCopyObj {
	public:
		NoCopyObj() {}
		virtual ~NoCopyObj() {}
		NoCopyObj(const NoCopyObj&) = delete;
		NoCopyObj& operator =(const NoCopyObj&) = delete;
	private:
	};

}

#endif // __NoCopyObj_HPP__
