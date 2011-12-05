#ifndef _KEY_HASH_H_
#define _KEY_HASH_H_

template <class K, class T>
class CSimpleHash
{
public:


	unsigned int (*m_hashFn)(const K& key);
	BOOL (*m_equalFn)(const K& key, const T& obj);

	struct MEMORYPOOL* m_pool;
	UINT32 m_count;
	UINT32 m_size;
	PList<T>** hash_arr;
	BOOL m_init;
	T m_default;

public:
	CSimpleHash()
	{
		m_init = FALSE;
		hash_arr = NULL;
	}

	~CSimpleHash()
	{
		if (m_init == TRUE) {
			Destroy();
		}
	}

	BOOL Apply(BOOL (*fn)(T& item, void* arg), void* arg)
	{
		ASSERT_RETFALSE(m_init);
		ASSERT_RETFALSE(fn != NULL);
		for (UINT32 i = 0; i < m_size; i++) {
			if (hash_arr[i] != NULL) {
				PList<T>::USER_NODE* pNode = NULL;
				while ((pNode = hash_arr[i]->GetNext(pNode)) != NULL) {
					if ((*fn)(pNode->Value, arg) == FALSE) {
						return FALSE;
					}
				}
			}
		}
		return TRUE;
	}

	BOOL Init(struct MEMORYPOOL* pool, int hash_size, unsigned int (*hashFn)(const K& key), BOOL (*equalFn)(const K& key, const T& obj), const T& def)
	{
		ASSERT_RETFALSE(m_init == FALSE);
		ASSERT_RETFALSE(hash_size > 0);
		ASSERT_RETFALSE(hashFn != NULL);
		ASSERT_RETFALSE(equalFn != NULL);

		m_pool = pool;
		hash_arr = (PList<T>**)MALLOCZ(m_pool, sizeof(PList<T>*) * hash_size);
		ASSERT_RETFALSE(hash_arr != NULL);
		for (INT32 i = 0; i < hash_size; i++) {
			hash_arr[i] = NULL;
		}

		m_size = hash_size;
		m_hashFn = hashFn;
		m_equalFn = equalFn;
		m_count = 0;
		m_default = def;
		m_init = TRUE;

		return TRUE;
	}

	void Destroy(void (*destroyFn)(struct MEMORYPOOL* pool, T & item) = NULL)
	{
		ASSERT_RETURN(m_init == TRUE);
		ASSERT_RETURN(hash_arr != NULL);
		for (UINT32 iii = 0; iii < m_size; iii++) {
			if (hash_arr[iii] != NULL) {
				if (destroyFn != NULL) {
					hash_arr[iii]->Destroy(m_pool, destroyFn);
				} else {
					hash_arr[iii]->Destroy(m_pool);
				}
				FREE(m_pool, hash_arr[iii]);
			}
		}

		FREE(m_pool, hash_arr);
		hash_arr = NULL;
		m_init = FALSE;
	}

	
	T& GetItem(const K& key)
	{
		if (hash_arr == NULL) {
			return m_default;
		}

		unsigned int id;
		ASSERT_RETVAL(m_init == TRUE, m_default);

		id = (*m_hashFn)(key) % m_size;
		if (hash_arr[id] == NULL) {
			return m_default;
		} else {
			PList<T>::USER_NODE* cur = hash_arr[id]->GetNext(NULL);
			while (cur != NULL) {
				if ((*m_equalFn)(key, cur->Value)) {
					return cur->Value;
				} else {
					cur = hash_arr[id]->GetNext(cur);
				}
			}
			return m_default;
		}
	}

	void AddItem(const K& key, const T& item)
	{
		unsigned int id;
		PList<T>::USER_NODE* cur;
		ASSERT_RETURN(m_init == TRUE);

		id = (*m_hashFn)(key) % m_size;
		if (hash_arr[id] == NULL) {
			hash_arr[id] = (PList<T>*)MALLOCZ(m_pool, sizeof(PList<T>));
			ASSERT_RETURN(hash_arr[id] != NULL);

			hash_arr[id]->Init();
		}

		for (cur = hash_arr[id]->GetNext(NULL); cur != NULL; cur = hash_arr[id]->GetNext(cur)) {
			if ((*m_equalFn)(key, cur->Value)) {
				hash_arr[id]->RemoveCurrent(cur);
				m_count--;
				break;
			}
		}
		hash_arr[id]->PListPushHeadPool(m_pool, item);
		cur = hash_arr[id]->GetNext(NULL);
		m_count++;
	}

	void RemoveItem(const K& key)
	{
		unsigned int id;
		PList<T>::USER_NODE* cur;
		ASSERT_RETURN(m_init == TRUE);

		id = (*m_hashFn)(key) % m_size;

		if (hash_arr[id] == NULL) {
			return;
		}

		for (cur = hash_arr[id]->GetNext(NULL); cur != NULL; cur = hash_arr[id]->GetNext(cur)) {
			if ((*m_equalFn)(key, cur->Value)) {
				hash_arr[id]->RemoveCurrent(cur);
				m_count--;
				return;
			}
		}
	}

	unsigned int GetCount()
	{
		ASSERT_RETZERO(m_init == TRUE);

		return m_count;
	}
};

#endif
