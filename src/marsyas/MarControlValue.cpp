/*
** Copyright (C) 1998-2006 George Tzanetakis <gtzan@cs.uvic.ca>
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software 
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include "MarControlValue.h"
#include "MarControl.h"
#include "MarControlManager.h"

#include <algorithm>

using namespace std;
using namespace Marsyas;

/************************************************************************/
/* MarControlValue implementation                                       */
/************************************************************************/
#ifdef MARSYAS_DEBUG
void
MarControlValue::setDebugValue()
{
	ostringstream oss;
	serialize(oss);

#ifdef MARSYAS_QT
	QWriteLocker(&rwLock_);
#endif

	value_debug_ = oss.str();
}
#endif

void
MarControlValue::addLink(MarControl* newlink)
{
	#ifdef MARSYAS_QT
	rwLock_.lockForWrite(); //[!]
	#endif

	lit_ = find(links_.begin(), links_.end(), newlink);
	if(lit_ != links_.end())
	{
		MRSWARN("MarControlValue::addLink() - link already exists!");
		#ifdef MARSYAS_QT
		rwLock_.unlock(); //[!]
		#endif
		return;
	}
	links_.push_back(newlink);

	#ifdef MARSYAS_QT
	rwLock_.unlock(); //[!]
	#endif
}

void
MarControlValue::removeLink(MarControl* link)
{
#ifdef MARSYAS_QT
	rwLock_.lockForWrite(); //[!]
#endif

	lit_ = find(links_.begin(), links_.end(), link);
	if(lit_ == links_.end())
	{
		MRSWARN("MarControlValue::removeLink() - trying to remove a non-link MarControl!");
		#ifdef MARSYAS_QT
		rwLock_.unlock(); //[!]
		#endif
		return;
	}
	links_.erase(lit_);

	//check if there is at least one link,
	//otherwise, no one else is using this object
	//meaning it should be destructed
	if(links_.size() == 0)
	{
		#ifdef MARSYAS_QT
		rwLock_.unlock(); //[!]
		#endif
		delete this;
		return;
	}

#ifdef MARSYAS_QT
	rwLock_.unlock(); //[!]
#endif
}

void
MarControlValue::callMarSystemsUpdate()
{
#ifdef MARSYAS_QT
	rwLock_.lockForRead(); //[!]
#endif

	//iterate over all the MarControls that own this MarControlValue
	//and call any necessary MarSystem updates after this value change
	for(lit_ = links_.begin(); lit_ != links_.end(); ++lit_)
		(*lit_)->callMarSystemUpdate(); //lit_ is a pointer to a MarControl*

#ifdef MARSYAS_QT
	rwLock_.unlock(); //[!]
#endif
}

string
MarControlValue::getType() const
{
	return type_;
}

string
MarControlValue::getRegisteredType()
{
	return MarControlManager::getManager()->getRegisteredType(this->getTypeID());
}

/************************************************************************/
/* MarControlValueT realvec specialization                              */
/************************************************************************/
realvec MarControlValueT<realvec>::invalidValue;

// constructor specialization for realvec
MarControlValueT<realvec>::MarControlValueT(realvec value)
{
	value_ = value;
	type_ = "mrs_realvec";

#ifdef MARSYAS_DEBUG
	setDebugValue();
#endif
}

MarControlValueT<realvec>::MarControlValueT(const MarControlValueT& a):MarControlValue(a)
{
	#ifdef MARSYAS_QT
	a.rwLock_.lockForRead(); //[!]
	#endif

	value_ = a.value_;
	type_ = "mrs_realvec";

#ifdef MARSYAS_DEBUG
	setDebugValue();
#endif

	#ifdef MARSYAS_QT
	a.rwLock_.unlock(); //[!]
	#endif
}

MarControlValueT<realvec>& 
MarControlValueT<realvec>::operator=(const MarControlValueT& a)
{
	if (this != &a)
	{
		#ifdef MARSYAS_QT
		a.rwLock_.lockForRead(); //[!]
		#endif

		value_ = a.value_;
		type_ = a.type_;

#ifdef MARSYAS_DEBUG
		setDebugValue();
#endif

		#ifdef MARSYAS_QT
		a.rwLock_.lockForRead(); //[!]
		#endif
	}
	return *this;
}

MarControlValue*
MarControlValueT<realvec>::clone()
{
	return new MarControlValueT<realvec>(*this);
}

void
MarControlValueT<realvec>::copyValue(MarControlValue& value)
{
	MarControlValueT<realvec> &v = dynamic_cast<MarControlValueT<realvec>&>(value);
	value_ = v.value_;
}

MarControlValue*
MarControlValueT<realvec>::create()
{
	return new MarControlValueT<realvec>(realvec());
}

const realvec&
MarControlValueT<realvec>::get() const
{
#ifdef MARSYAS_QT
	QReadLocker locker(&rwLock_); //[!]
#endif 

	return value_;
}

realvec&
MarControlValueT<realvec>::getRef()
{
#ifdef MARSYAS_QT
	QReadLocker locker(&rwLock_); //[!]
#endif 

	return value_;
}

bool
MarControlValueT<realvec>::isNotEqual(MarControlValue *v)
{
	if(this != v)//if referring to different objects, check if their contents is different...
	{
		if (type_ != "mrs_realvec")
		{
			MRSWARN("MarControlValueT::isNotEqual() - Types of MarControlValue are different");
		}

		#ifdef MARSYAS_QT
		rwLock_.lockForRead(); //[!]
		#endif

		bool res = (value_ != dynamic_cast<MarControlValueT<realvec>*>(v)->get());

		#ifdef MARSYAS_QT
		rwLock_.unlock(); //[!]
		#endif

		return res;//value_ != dynamic_cast<MarControlValueT<realvec>*>(v)->get();
	}
	else //if v1 and v2 refer to the same object, they must be equal (=> return false)
		return false;
}

void
MarControlValueT<realvec>::createFromStream(std::istream& in)
{
#ifdef MARSYAS_QT
	rwLock_.lockForWrite(); //[!]
#endif

	in >> value_;

#ifdef MARSYAS_QT
	rwLock_.unlock(); //[!]
#endif

#ifdef MARSYAS_DEBUG
	setDebugValue();
#endif
}

std::ostream&
MarControlValueT<realvec>::serialize(std::ostream& os)
{
#ifdef MARSYAS_QT
	rwLock_.lockForRead(); //[!]
#endif
	
	os << value_;

#ifdef MARSYAS_QT
	rwLock_.unlock(); //[!]
#endif
	
	return os;
}

MarControlValue*
MarControlValueT<realvec>::sum(MarControlValue *v)
{
	(void) v;
	MRSASSERT(0); //not implemented
	return 0;
}

MarControlValue*
MarControlValueT<realvec>::subtract(MarControlValue *v)
{
	(void) v;
	MRSASSERT(0); //not implemented
	return 0;
}

MarControlValue*
MarControlValueT<realvec>::multiply(MarControlValue *v)
{
	(void) v;
	MRSASSERT(0); //not implemented
	return 0;
}

MarControlValue*
MarControlValueT<realvec>::divide(MarControlValue *v)
{
	(void) v;
	MRSASSERT(0); //not implemented
	return 0;
}

/************************************************************************/
/* MarControlValueT bool specialization                                 */
/************************************************************************/
bool MarControlValueT<bool>::invalidValue;

// constructor specialization for realvec
MarControlValueT<bool>::MarControlValueT(bool value)
{
	value_ = value;
	type_ = "mrs_bool";

#ifdef MARSYAS_DEBUG
	setDebugValue();
#endif
}

MarControlValueT<bool>::MarControlValueT(const MarControlValueT& a):MarControlValue(a)
{
#ifdef MARSYAS_QT
	a.rwLock_.lockForRead(); //[!]
#endif

	value_ = a.value_;
	type_ = "mrs_bool";

#ifdef MARSYAS_DEBUG
	setDebugValue();
#endif

#ifdef MARSYAS_QT
	a.rwLock_.unlock(); //[!]
#endif
}

MarControlValueT<bool>& 
MarControlValueT<bool>::operator=(const MarControlValueT& a)
{
	if (this != &a)
	{
#ifdef MARSYAS_QT
		a.rwLock_.lockForRead(); //[!]
#endif

		value_ = a.value_;
		type_ = a.type_;

#ifdef MARSYAS_DEBUG
		setDebugValue();
#endif

#ifdef MARSYAS_QT
		a.rwLock_.unlock(); //[!]
#endif
	}
	return *this;
}

MarControlValue*
MarControlValueT<bool>::clone()
{
	return new MarControlValueT<bool>(*this);
}

void
MarControlValueT<bool>::copyValue(MarControlValue& value)
{
	MarControlValueT<bool> &v = dynamic_cast<MarControlValueT<bool>&>(value);
	value_ = v.value_;
}

MarControlValue*
MarControlValueT<bool>::create()
{
	return new MarControlValueT<bool>(false);
}

const bool&
MarControlValueT<bool>::get() const
{
#ifdef MARSYAS_QT
	QReadLocker locker(&rwLock_); //[!]
#endif 

	return value_;
}

void
MarControlValueT<bool>::createFromStream(std::istream& in)
{
#ifdef MARSYAS_QT
	rwLock_.lockForWrite(); //[!]
#endif

	in >> value_;

#ifdef MARSYAS_DEBUG
	setDebugValue();
#endif

#ifdef MARSYAS_QT
	rwLock_.unlock(); //[!]
#endif
}

bool
MarControlValueT<bool>::isNotEqual(MarControlValue *v)
{
	if(this != v)//if referring to different objects, check if their contents is different...
	{
#ifdef MARSYAS_QT
		rwLock_.lockForRead(); //[!]
#endif
		
		bool res = (value_ != dynamic_cast<MarControlValueT<bool>*>(v)->get());

#ifdef MARSYAS_QT
		rwLock_.unlock(); //[!]
#endif

	  return res;//value_ != dynamic_cast<MarControlValueT<bool>*>(v)->get();
	}
	else //if v1 and v2 refer to the same object, they must be equal (=> return false)
	  return false;
}

std::ostream&
MarControlValueT<bool>::serialize(std::ostream& os)
{
#ifdef MARSYAS_QT
	rwLock_.lockForRead(); //[!]
#endif

	os << value_;
	
#ifdef MARSYAS_QT
	rwLock_.unlock(); //[!]
#endif

	return os;
}

MarControlValue*
MarControlValueT<bool>::sum(MarControlValue *v)
{
	(void) v;
	MRSASSERT(0); //not implemented
	return 0;
}

MarControlValue*
MarControlValueT<bool>::subtract(MarControlValue *v)
{
	(void) v;
	MRSASSERT(0); //not implemented
	return 0;
}

MarControlValue*
MarControlValueT<bool>::multiply(MarControlValue *v)
{
	(void) v;
	MRSASSERT(0); //not implemented
	return 0;
}

MarControlValue*
MarControlValueT<bool>::divide(MarControlValue *v)
{
	(void) v;
	MRSASSERT(0); //not implemented
	return 0;
}

