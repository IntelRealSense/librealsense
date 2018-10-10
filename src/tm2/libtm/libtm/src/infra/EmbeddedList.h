/*******************************************************************************
INTEL CORPORATION PROPRIETARY INFORMATION
Copyright(c) 2017 Intel Corporation. All Rights Reserved.
*******************************************************************************/

#pragma once


namespace perc {
// ----------------------------------------------------------------------------
///
/// @class  EmbeddedList
///
/// @brief  Doubly linked list.
///
/// @note   This implementation of a doubly linked list does not require
///         use of dynamically allocated memory. Instead, each class
///         that is a potential list element must inherite from a class
///         <EmbeddedListElement>. All of the list functions operate on these
///         embedded element.
///
// ----------------------------------------------------------------------------
class EmbeddedListElement
{
public:
    EmbeddedListElement *Next () const {return m_EmbeddedListElementNext; }
    void Next (EmbeddedListElement *Element) { m_EmbeddedListElementNext = Element; }
    EmbeddedListElement *Prev () const {return m_EmbeddedListElementPrev; }
    void Prev (EmbeddedListElement *Element) { m_EmbeddedListElementPrev = Element; }
private:
    EmbeddedListElement *m_EmbeddedListElementNext;
    EmbeddedListElement *m_EmbeddedListElementPrev;
};

class EmbeddedList
{
public:

    EmbeddedList () : Head (0), Tail (0), m_Size(0) {}

    // Check Head
    EmbeddedListElement *GetHead ()
    {
        return Head;
    }

    // Inserts an element onto the top of the list
    void AddHead (EmbeddedListElement *Element)
    {
        //ASSERT (Element);
        Element->Prev (0);
        Element->Next (Head);

        // if first element, the tail also points to this element
        if (!Head)
            Tail = Element;
        // more than one
        else
            // poin to the new head
            Head->Prev (Element);
        Head = Element;
        m_Size++;
    }

    // Takes a element off from the top of the list
    EmbeddedListElement *RemoveHead ()
    {
        EmbeddedListElement *element = Head;
        if (element)
        {
            // if only one element
            if (Tail == Head)
                Tail = 0;
            else
                element->Next ()->Prev (0);
            Head = element->Next ();
            m_Size--;
        }
        return element;
    }

    // Inserts a element onto the end of the list
    void AddTail (EmbeddedListElement *Element)
    {
        //ASSERT (Element);
        Element->Prev (Tail);
        Element->Next (0);

        // if first element, the tail also points to this element
        if (!Tail)
            Head = Element;
        // more than one
        else
            // poin to the new tail
            Tail->Next (Element);
        Tail = Element;
        m_Size++;
    }

    // Takes a element off from the end of the list
    EmbeddedListElement *RemoveTail ()
    {
        EmbeddedListElement *element = Tail;
        if (element)
        {
            // if only one element
            if (Tail == Head)
                Head = 0;
            else
                element->Prev ()->Next (0);
            Tail = element->Prev ();
            m_Size--;
        }
        return element;
    }

    // Inserts a element onto the end of the list
    void AddBefore (EmbeddedListElement *ElementCurr, EmbeddedListElement *ElementNew)
    {
        //ASSERT (ElementCurr && ElementNew);
        if (ElementCurr == Head)
            AddHead (ElementNew);
        else
        {
            ElementNew->Prev (ElementCurr->Prev ());
            ElementCurr->Prev (ElementNew);
            ElementNew->Next (ElementCurr);
            ElementNew->Prev ()->Next (ElementNew);
            m_Size++;
        }
    }

    // Takes a element off from the list
    int Remove (EmbeddedListElement *Element)
    {
        //ASSERT (Element);
        if (Element == Head)
            RemoveHead ();
        else if (Element == Tail)
            RemoveTail ();
        else
        {
            Element->Next ()->Prev (Element->Prev ());
            Element->Prev ()->Next (Element->Next ());
            m_Size--;
        }
        return 0;
    }

    /// @class Iterator
    class Iterator
    {
        const EmbeddedList  &m_List;
        EmbeddedListElement *m_CurrentNode;
    public:
        Iterator (const EmbeddedList &List) : m_List (List) { Reset (); }

        // Reset the Iterator so that GetNext will give you the first list element
        void Reset ()
        {
            m_CurrentNode = m_List.Head;
        }

        // Return the next list element (i.e. advance iterator and return next list element)
        EmbeddedListElement *Next ()
        {
            // move the iterator one step forward
            if (m_CurrentNode)
                m_CurrentNode = m_CurrentNode->Next ();
            return m_CurrentNode;
        }

        // Return the current list element
        EmbeddedListElement *Current () const { return m_CurrentNode; }
    };

    int Size() const { return m_Size; }

private:
    EmbeddedListElement     *Head;
    EmbeddedListElement     *Tail;
    int                     m_Size;
};


// ----------------------------------------------------------------------------
} // namespace perc
