#include "patchstd.h"
#include "simple_hash.h"
/*
template <class K, class T>
CSimpleHash<K, T>::CSimpleHash<K, T>()
{
	m_init = FALSE;
	hash_arr = NULL;
}

template <class K, class T>
CSimpleHash<K, T>::~CSimpleHash<K, T>()
{
	if (m_init == TRUE) {
		Destroy();
	}
}

template <class K, class T>
void CSimpleHash<K, T>::Init(struct MEMORYPOOL* pool, int hash_size, 
						  int (*hashFn)(const K& key),
						  BOOL (*equalFn)(const K& key, const T& obj),
						  const T& def)
{
	ASSERT(m_init == FALSE);
	ASSERT(hash_size > 0);
	ASSERT(hashFn != NULL);
	ASSERT(equalFn != NULL);

	m_pool = pool;
	hash_arr = (CList<T>**)MALLOCZ(m_pool, sizeof(CList<T>*) * hash_size);
	ASSERT(hash_arr != NULL);
	memset(hash_arr, 0, sizeof(CList<T>*) * hash_size);

	m_size = hash_size;
	m_hashFn = hashFn;
	m_equalFn = equalFn;
	m_count = 0;
	m_default = def;

	m_init = TRUE;
}

template <class K, class T>
void CSimpleHash<K, T>::Destroy(void (*destroyFn)(struct MEMORYPOOL* pool, T item))
{
	ASSERT(m_init == TRUE);
	ASSERT(hash_arr != NULL);

	for (unsigned int i = 0; i < m_size; i++) {
		if (hash_arr[i] != NULL) {
			if (destroyFn != NULL) {
				hash_arr[i]->Destroy(m_pool, destroyFn);
			} else {
				hash_arr[i]->Destroy(m_pool);
			}
			FREE(m_pool, hash_arr[i]);
		}
	}

	FREE(m_pool, hash_arr);
	hash_arr = NULL;
	m_init = FALSE;
}

template <class K, class T>
T& CSimpleHash<K, T>::GetItem(const K& key)
{
	int id;
	ASSERT(m_init == TRUE);

	id = (*m_hashFn)(key);
	ASSERT (id >= 0 && id < m_size);

	if (hash_arr[id] == NULL) {
		return m_default;
	} else {
		CList<T>::USER_NODE* cur = hash_arr[id]->GetNext(NULL);
		while (cur != NULL) {
			if ((*m_equalFn)(key, cur->Value)) {
				return cur->Value;
			} else {
				cur = pList->GetNext(cur);
			}
		}
		return m_default;
	}
}

template <class K, class T>
void CSimpleHash<K, T>::AddItem(const K& key, const T& item)
{
	int id;
	CList<T>::USER_NODE* cur;
	ASSERT(m_init == TRUE);

	id = (*m_hashFn)(key);
	ASSERT (id >= 0 && id < m_size);

	if (hash_arr[id] == NULL) {
		hash_arr[id] = (CList<T>*)MALLOCZ(m_pool, sizeof(CList<T>));
		ASSERT(hash_arr[id] != NULL);

		hash_arr[id]->Init();
	}

	for (cur = hash_arr[id]->GetNext(NULL); cur != NULL; cur = hash_arr[id]->GetNext(cur)) {
		if ((*m_equalFn)(key, cur->Value)) {
			hash_arr[id]->RemoveCurrent(cur);
			m_count--;
			break;
		}
	}
	hash_arr[id]->PushHead(m_pool, item);
	m_count++;
}

template <class K, class T>
void CSimpleHash<K, T>::RemoveItem(const K& key)
{
	int id;
	CList<T>::USER_NODE* cur;
	ASSERT(m_init == TRUE);

	id = (*m_hashFn)(key);
	ASSERT (id >= 0 && id < m_size);

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

template <class K, class T>
unsigned int CSimpleHash<K, T>::GetCount()
{
	ASSERT(m_init == TRUE);

	return m_count;
}
*/
