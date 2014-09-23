// Copyright (c) 2010, Lawrence Livermore National Security, LLC. Produced at
// the Lawrence Livermore National Laboratory. LLNL-CODE-443211. All Rights
// reserved. See file COPYRIGHT for details.
//
// This file is part of the MFEM library. For more information and source code
// availability see http://mfem.googlecode.com.
//
// MFEM is free software; you can redistribute it and/or modify it under the
// terms of the GNU Lesser General Public License (as published by the Free
// Software Foundation) version 2.1 dated February 1999.

#ifndef MFEM_HASH
#define MFEM_HASH

#include "idgenerator.hpp"


/** A concept for items that should be used in HashTable and be accessible by
 *  hashing two IDs.
 *
 *  NOTE: the CRTP pattern is needed for correct pointer arithmetics if the
 *        derived class uses multiple inheritance.
 */
template<typename Derived>
struct Hashed2
{
   int id;
   int p1, p2;
   Derived* next;

   Hashed2(int id) : id(id) {}
};

/** A concept for items that should be used in HashTable and be accessible by
 *  hashing four IDs.
 */
template<typename Derived>
struct Hashed4
{
   int id;
   int p1, p2, p3; // NOTE: p4 is not hashed nor stored
   Derived* next;

   Hashed4(int id) : id(id) {}
};


/** HashTable is a container for items that require associative access through
 *  pairs (or quadruples) of indices:
 *
 *    (p1, p2) -> item
 *    (p1, p2, p3, p4) -> item
 *
 *  An example of this are edges and faces in a mesh. Each edge is uniquely
 *  identified by two parent vertices and so can be easily accessed from
 *  different elements using this class. Similarly with faces.
 *
 *  The order of the p1, p2, ... indices is not relevant as they are sorted
 *  each time this class is invoked.
 *
 *  There are two main methods this class provides. The Get(...) method always
 *  returns an item given the two or four indices. If the item didn't previously
 *  exist, the methods creates a new one. The Peek(...) method, on the other
 *  hand, just returns NULL if the item doesn't exist.
 *
 *  Each new item is automatically assigned a unique ID. The IDs may (but
 *  need not) be used as p1, p2, ... of other items.
 *
 *  The item type (ItemT) needs to follow either the Hashed2 or the Hashed4
 *  concept. It is easiest to just inherit from these structs.
 *
 *  All items in the container can also be accessed sequentially using the
 *  provided iterator.
 *
 *  TODO: The hash table currently doesn't adapt its size to the number of
 *        items stored!
 */
template<typename ItemT>
class HashTable
{
public:
   HashTable(int size = 128*1024);
   ~HashTable();

   /// Get an item whose parents are p1, p2... Create it if it doesn't exist.
   ItemT* Get(int p1, int p2);
   ItemT* Get(int p1, int p2, int p3, int p4);

   /// Get an item whose parents are p1, p2... Return NULL if it doesn't exist.
   ItemT* Peek(int p1, int p2) const;
   ItemT* Peek(int p1, int p2, int p3, int p4) const;

   // item pointer variants of the above for convenience
   template<typename OtherT>
   ItemT* Get(OtherT* i1, OtherT* i2)
      { return Get(i1->id, i2->id); }

   template<typename OtherT>
   ItemT* Get(OtherT* i1, OtherT* i2, OtherT* i3, OtherT* i4)
      { return Get(i1->id, i2->id, i3->id, i4->id); }

   template<typename OtherT>
   ItemT* Peek(OtherT* i1, OtherT* i2) const
      { return Peek(i1->id, i2->id); }

   template<typename OtherT>
   ItemT* Peek(OtherT* i1, OtherT* i2, OtherT* i3, OtherT* i4) const
      { return Peek(i1->id, i2->id, i3->id, i4->id); }

   /// Obtains an item given its ID.
   ItemT* Peek(int id) const { return id_to_item[id]; }

   /// Remove an item from the hash table and also delete the item itself.
   void Delete(ItemT* item);
   void Delete(int id) { Delete(Peek(id)); }

   /// Iterator over items contained in the HashTable.
   class Iterator
   {
   public:
      Iterator(HashTable<ItemT>& table)
         : hash_table(table), next_bin(0), cur_item(NULL) { next(); }

      operator ItemT*() const { return cur_item; }
      ItemT& operator*() const { return *cur_item; }
      ItemT* operator->() const { return cur_item; }

      Iterator &operator++() { next(); return *this; }

   protected:
      HashTable<ItemT>& hash_table;
      int next_bin;
      ItemT* cur_item;

      void next();
   };

protected:

   ItemT** table;
   int mask;

   // hash functions (NOTE: the constants are arbitrary)
   inline int hash(int p1, int p2) const
      { return (984120265*p1 + 125965121*p2) & mask; }

   inline int hash(int p1, int p2, int p3) const
      { return (984120265*p1 + 125965121*p2 + 495698413*p3) & mask; }

   // Remove() uses one of the following two overloads:
   inline int hash(const Hashed2<ItemT>* item) const
      { return hash(item->p1, item->p2); }

   inline int hash(const Hashed4<ItemT>* item) const
      { return hash(item->p1, item->p2, item->p3); }

   ItemT* SearchList(ItemT* item, int p1, int p2) const;
   ItemT* SearchList(ItemT* item, int p1, int p2, int p3) const;

   IdGenerator id_gen; ///< id generator for new items
   Array<ItemT*> id_to_item; ///< mapping table for the Peek(id) method

   Array<int> used_bins; ///< bins in 'table' that (may) contain something
   char* new_bin; ///< flag showing that bin index is not in 'used_bins'

   mutable int nqueries, ncollisions; // stats
};


// implementation

template<typename ItemT>
HashTable<ItemT>::HashTable(int size)
{
   mask = size-1;
   if (size & mask)
      mfem_error("Hash table size must be a power of two.");

   table = new ItemT*[size];
   memset(table, 0, size * sizeof(ItemT*));

   new_bin = new char[size];
   memset(new_bin, 1, size * sizeof(char));

   nqueries = ncollisions = 0;
}

template<typename ItemT>
HashTable<ItemT>::~HashTable()
{
   // delete all items
   for (Iterator it(*this); it; )
   {
      ItemT* tmp = it;
      ++it;
      delete tmp;
   }

   delete [] table;
   delete [] new_bin;

   if (ncollisions > 3*nqueries)
      std::cout << "HashTable: WARNING: ncollisions = " << ncollisions << ", "
                << "nqueries = " << nqueries << ". Adaptive resizing needed.";
}

namespace detail {

inline void sort3(int &a, int &b, int &c)
{
   if (a > b) std::swap(a, b);
   if (a > c) std::swap(a, c);
   if (b > c) std::swap(b, c);
}

inline void sort4(int &a, int &b, int &c, int &d)
{
   if (a > b) std::swap(a, b);
   if (a > c) std::swap(a, c);
   if (a > d) std::swap(a, d);
   sort3(b, c, d);
}

} // detail

template<typename ItemT>
ItemT* HashTable<ItemT>::Peek(int p1, int p2) const
{
  if (p1 > p2) std::swap(p1, p2);
  return SearchList(table[hash(p1, p2)], p1, p2);
}

template<typename ItemT>
ItemT* HashTable<ItemT>::Peek(int p1, int p2, int p3, int p4) const
{
  detail::sort4(p1, p2, p3, p4);
  return SearchList(table[hash(p1, p2, p3)], p1, p2, p3);
}

template<typename ItemT>
ItemT* HashTable<ItemT>::Get(int p1, int p2)
{
  // search for the item in the hashtable
  if (p1 > p2) std::swap(p1, p2);
  int idx = hash(p1, p2);
  ItemT* node = SearchList(table[idx], p1, p2);
  if (node) return node;

  // not found - create a new one
  ItemT* newitem = new ItemT(id_gen.Get());
  newitem->p1 = p1;
  newitem->p2 = p2;

  // insert into hashtable
  newitem->next = table[idx];
  table[idx] = newitem;

  // if this is a new bin, make sure the iterator will find it
  if (new_bin[idx])
  {
     used_bins.Append(idx);
     new_bin[idx] = 0;
  }

  // also, maintain the mapping ID -> item
  if (id_to_item.Size() <= newitem->id) {
     id_to_item.SetSize(newitem->id + 1, NULL);
  }
  id_to_item[newitem->id] = newitem;

  return newitem;
}

template<typename ItemT>
ItemT* HashTable<ItemT>::Get(int p1, int p2, int p3, int p4)
{
  // search for the item in the hashtable
  detail::sort4(p1, p2, p3, p4);
  int idx = hash(p1, p2, p3);
  ItemT* node = SearchList(table[idx], p1, p2, p3);
  if (node) return node;

  // not found - create a new one
  ItemT* newitem = new ItemT(id_gen.Get());
  newitem->p1 = p1;
  newitem->p2 = p2;
  newitem->p3 = p3;

  // insert into hashtable
  newitem->next = table[idx];
  table[idx] = newitem;

  // if this is a new bin, make sure the iterator will find it
  if (new_bin[idx])
  {
     used_bins.Append(idx);
     new_bin[idx] = 0;
  }

  // also, maintain the mapping ID -> item
  if (id_to_item.Size() <= newitem->id) {
     id_to_item.SetSize(newitem->id + 1, NULL);
  }
  id_to_item[newitem->id] = newitem;

  return newitem;
}

template<typename ItemT>
ItemT* HashTable<ItemT>::SearchList(ItemT* item, int p1, int p2) const
{
   nqueries++;
   while (item != NULL)
   {
      if (item->p1 == p1 && item->p2 == p2) return item;
      item = item->next;
      ncollisions++;
   }
   return NULL;
}

template<typename ItemT>
ItemT* HashTable<ItemT>::SearchList(ItemT* item, int p1, int p2, int p3) const
{
   nqueries++;
   while (item != NULL)
   {
      if (item->p1 == p1 && item->p2 == p2 && item->p3 == p3) return item;
      item = item->next;
      ncollisions++;
   }
   return NULL;
}

template<typename ItemT>
void HashTable<ItemT>::Delete(ItemT* item)
{
   int idx = hash(item);

   // remove item from the hash table
   ItemT** ptr = table + idx;
   while (*ptr)
   {
      if (*ptr == item)
      {
         *ptr = item->next;
         goto ok;
      }
      ptr = &((*ptr)->next);
   }
   mfem_error("HashTable<>::Delete: item not found!");

ok:
   // remove from the (ID -> item) map
   id_to_item[item->id] = NULL;

   // reuse the item ID in the future
   id_gen.Reuse(item->id);

   delete item;
}

template<typename ItemT>
void HashTable<ItemT>::Iterator::next()
{
   if (next_bin >= hash_table.used_bins.Size())
   {
      // no more bins to visit, finish
      cur_item = NULL;
      return;
   }

   if (cur_item)
   {
      // iterate through a list of hash synonyms
      cur_item = cur_item->next;
   }

   // do we need to switch the next bin?
   while (!cur_item && next_bin < hash_table.used_bins.Size())
   {
      cur_item = hash_table.table[hash_table.used_bins[next_bin++]];
   }
}

#endif
