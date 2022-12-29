//
// state.hpp
// add by [11/21/2022 cuihaoyun]
// 对象池用的模板节点
//

#ifndef __NODE_HPP__
#define __NODE_HPP__

namespace asio {

	template <typename T>
	class Node
	{
	public:
		Node() {}
		virtual ~Node() {}
		Node(const Node& other) {
			this->next_ = other.next_;
			this->prev_ = other.prev_;
		}

		Node& operator=(const Node& other) {
			if (this == &other)
				return *this;

			this->next_ = other.next_;
			this->prev_ = other.prev_;
			return *this;
		}

		T* next_ = { nullptr };
		T* prev_ = { nullptr };
	};


	//-------------------------------------------------------
	// example
	/*
	* class User : public Node<User>
	*{
	*public:
	*	User(){}
	*	~User(){}
	*	void SetValue(int v) {
	*		this->value = v;
	*	}
	*	int GetValue() {
	*		return this->value;
	*	}
	*protected:
	*private:
	*	int value;
	*};
	* void main(){
	*	object_pool<User> objectPool;
	*	User* pUser1 = objectPool.alloc();
	*	User* pUser2 = objectPool.alloc();
	*	User* pUser3 = objectPool.alloc();
	*
	*	objectPool.free(pUser1);
	*	User* p = objectPool.alloc();
	* }
	*/
}


#endif // __NODE_HPP__
