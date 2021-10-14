//for cling
#include <iostream>
#include <vector>

template<typename T>
class vector_ext:public std::vector<T>{
public:
	vector_ext<T>& push(const T& val){
		this->push_back(val);
		return *this;
	}
};

template<typename T>
vector_ext<T>& operator++(std::vector<T> &a,int){
	return *(vector_ext<T>*)&a;
}

int test_vector_ext(){
	std::vector<int> a;
	a++.push(3);
	std::cout<<a.size()<<std::endl;
	return 0;
}
