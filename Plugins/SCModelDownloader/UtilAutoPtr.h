#pragma once

template<typename T>
class UtilAutoPtr {
private:
	T* m_ptr;

public:
	UtilAutoPtr() : m_ptr(nullptr) {}

	explicit UtilAutoPtr(T* ptr) : m_ptr(ptr) {
		if (m_ptr) {
			m_ptr->AddRef();
		}
	}

	UtilAutoPtr(const UtilAutoPtr& other) : m_ptr(other.m_ptr) {
		if (m_ptr) {
			m_ptr->AddRef();
		}
	}

	UtilAutoPtr(UtilAutoPtr&& other) noexcept : m_ptr(other.m_ptr) {
		other.m_ptr = nullptr;
	}

	~UtilAutoPtr() {
		if (m_ptr) {
			m_ptr->Release();
		}
	}

	UtilAutoPtr& operator=(const UtilAutoPtr& other) {
		if (this != &other) {
			if (m_ptr) {
				m_ptr->Release();
			}
			m_ptr = other.m_ptr;
			if (m_ptr) {
				m_ptr->AddRef();
			}
		}
		return *this;
	}

	UtilAutoPtr& operator=(UtilAutoPtr&& other) noexcept {
		if (this != &other) {
			if (m_ptr) {
				m_ptr->Release();
			}
			m_ptr = other.m_ptr;
			other.m_ptr = nullptr;
		}
		return *this;
	}

	T* operator->() const { return m_ptr; }
	T& operator*() const { return *m_ptr; }
	operator T* () const { return m_ptr; }

	T* Get() const { return m_ptr; }

	void Reset(T* ptr = nullptr) {
		if (m_ptr) {
			m_ptr->Release();
		}
		m_ptr = ptr;
		if (m_ptr) {
			m_ptr->AddRef();
		}
	}

	T* Detach() {
		T* ptr = m_ptr;
		m_ptr = nullptr;
		return ptr;
	}
};