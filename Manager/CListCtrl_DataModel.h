#pragma once

#include <windows.h>
#include <common.h>

struct CListCtrl_DataRecord
{
	CListCtrl_DataRecord()
	{}

	CListCtrl_DataRecord(const CString& id, const CString& desc, CString state, CString device, PMR_IO_SLOT slot)
		:m_Id(id)
		,m_Desc(desc)
        ,m_State(state)
		,m_Device(device)
	{
        m_Slot = slot;
    }

	CString	m_Desc;
	CString	m_Id;
	CString m_Device;
    CString m_State;
    PMR_IO_SLOT m_Slot;

	PMR_IO_SLOT GetIoSlot() const
	{
        return m_Slot;
    }

	CString GetCellText(int col, bool title) const
	{
		switch(col)
		{
		case 0: { static const CString title0(_T("Mouse Description")); return title ? title0 : m_Id; }
		case 1: { static const CString title1(_T("Hardware ID")); return title ? title1 : m_Desc; }
		case 2: { static const CString title2(_T("Swap Buttons")); return title ? title2 : m_State; }
		case 3: { static const CString title3(_T("Device Object")); return title ? title3 : m_Device; }
		default:{ static const CString emptyStr; return emptyStr; }
		}
	}

	int  GetColCount() const { return 4; }
};

class CListCtrl_DataModel
{
private:

	vector<CListCtrl_DataRecord> m_Records;
	int	m_LookupTime;
	int m_RowMultiplier;

    struct mr_io_record *m_ior;
    unsigned long        m_iol;

public:

    ~CListCtrl_DataModel()
    {
        if (m_ior)
            delete []m_ior;
    }

	CListCtrl_DataModel()
		:m_RowMultiplier(0)
		,m_LookupTime(0)
	{
        m_iol = 128 * 1024;
        m_ior = (struct mr_io_record *)(new char[m_iol]);

		InitDataModel();
	}

    int ReadKernelMoure(PMR_IO_RECORD mir, ULONG len)
    {
	    HANDLE  h = 0;
	    DWORD   bytes = 0, le;
        BOOL    rc;

	    h = CreateFile( MOURE_DOSDEV,
		                GENERIC_READ | GENERIC_WRITE,
		                0,
		                NULL,
		                CREATE_ALWAYS,
		                FILE_ATTRIBUTE_NORMAL,
		                NULL);

	    if (h == INVALID_HANDLE_VALUE || h == 0) {
    		le = GetLastError();
		    goto errorout;
    	}
        memset(mir, 0, len);

        rc = ReadFile(h, mir, len, &bytes, NULL);
	    if (!rc) {
		    le = GetLastError();
		    goto errorout;
	    }

errorout:
	    if (h != INVALID_HANDLE_VALUE && h != NULL)
		    CloseHandle(h);

	    return (int) bytes;
    }

    int WriteKernelMoure(PMR_IO_RECORD mir, ULONG len)
    {
	    HANDLE  h = 0;
	    DWORD   bytes = 0, le;
        BOOL    rc;

	    h = CreateFile( MOURE_DOSDEV,
		                GENERIC_READ | GENERIC_WRITE,
		                0,
		                NULL,
		                CREATE_ALWAYS,
		                FILE_ATTRIBUTE_NORMAL,
		                NULL);

	    if (h == INVALID_HANDLE_VALUE || h == 0) {
    		le = GetLastError();
		    goto errorout;
    	}

        rc = WriteFile(h, mir, len, &bytes, NULL);
	    if (!rc) {
		    le = GetLastError();
		    goto errorout;
	    }

errorout:
	    if (h != INVALID_HANDLE_VALUE && h != NULL)
		    CloseHandle(h);

	    return (int) bytes;
    }

    int RefreshLists()
    {
        CString     s1, s2, s3, s4;
        PWCHAR      t1, t2;
        ULONG       total = MR_IO_REC_SIZE;

        if (!ReadKernelMoure(m_ior, m_iol))
            return 0;

        if (m_ior->mi_magic  != MR_IO_MAGIC ||
            m_ior->mi_length <= MR_IO_REC_SIZE + MR_IO_SLOT_SIZE) {
            return 0;
        }

        while (total + MR_IO_SLOT_SIZE < m_ior->mi_length) {

            PMR_IO_SLOT slot = (PMR_IO_SLOT)((PUCHAR)m_ior + total);
            if (slot->mi_magic !=  MR_IO_SLOT_MAGIC ||
                slot->mi_size == 0 || slot->mi_dcb == 0) {
                break;
            }

            t1 = (PWCHAR)((PUCHAR)slot + slot->mi_desc.me_start);
            t2 = (PWCHAR)((PUCHAR)slot + slot->mi_ven.me_start);
            s1.Format(_T("%s (%s)"), t1, t2);

            t1 = (PWCHAR)((PUCHAR)slot + slot->mi_id.me_start);
            s2.Format(_T("%s"), t1);

            if (slot->mi_state & MR_DEV_STATE_ENABLED) {
                if (slot->mi_state & MR_DEV_STATE_PERSIST)
                    s3 = _T("Persistent");
                else
                    s3 = _T("Temporary");
            } else {
                s3.Empty();
            }

            t1 = (PWCHAR)((PUCHAR)slot + slot->mi_name.me_start);
            s4.Format(_T("%s"), t1);

            m_Records.push_back( CListCtrl_DataRecord(s1, s2, s3, s4, slot));
            total += slot->mi_size;
        }

        return m_Records.size();
    }

    int UpdateMouseState(int row, CString state)
    {
        PMR_IO_SLOT     slot, ts;
        PMR_IO_RECORD   mir = NULL;
        ULONG           len = 0;
        int             rc = 0;

        slot = GetIoSlot(row);
        if (NULL == slot)
            goto errorout;

        len = MR_IO_REC_SIZE + slot->mi_size;
        mir = (PMR_IO_RECORD)  (new char[len]);
        if (NULL == mir)
            goto errorout;

        memset(mir, 0, len);
        mir->mi_magic = MR_IO_MAGIC;
        mir->mi_length = len;
        mir->mi_nslots = 1;
        ts = &mir->mi_slot[0];
        memcpy(ts, slot, slot->mi_size);

        ts->mi_cmd = MR_IO_CMD_SET;
        ts->mi_state = 0;
        if (state.IsEmpty()) {
        } else if (state == _T("Persistent")) {
            ts->mi_state |= MR_DEV_STATE_ENABLED | 
                            MR_DEV_STATE_PERSIST;
        } else if (state == _T("Temporary")) {
            ts->mi_state |= MR_DEV_STATE_ENABLED;
        }

        if (ts->mi_state == slot->mi_state) {
            rc = 1;
            goto errorout;
        }

        /* update to kernel module */
        rc = WriteKernelMoure(mir, len);

        /* update registry for next booting */

errorout:
        if (mir)
            delete []mir;
        return rc;
    }

	void InitDataModel()
	{
		m_Records.clear();

        if (!RefreshLists())
            return;

		if (m_RowMultiplier > 1)
		{
			vector<CListCtrl_DataRecord> rowset(m_Records);
			m_Records.reserve((m_RowMultiplier-1) * rowset.size());
			for(int i = 0 ; i < m_RowMultiplier ; ++i)
			{
				m_Records.insert(m_Records.end(), rowset.begin(), rowset.end());
			}
		}
	}

	CString GetCellText(size_t lookupId, int col) const
	{
		if (lookupId >= m_Records.size())
		{
			static CString oob(_T("Out of Bound"));
			return oob;
		}
		// How many times should we search sequential for the row ?
		for(int i=0; i < m_LookupTime; ++i)
		{
			for(size_t rowId = 0; rowId < m_Records.size(); ++rowId)
			{
				if (rowId==lookupId)
					break;
			}
		}
		return m_Records.at(lookupId).GetCellText(col, false);
	}

	PMR_IO_SLOT GetIoSlot(size_t rowId) const
	{
		if (rowId >= m_Records.size())
		{
			return NULL;
		}

		return m_Records.at(rowId).GetIoSlot();
	}

	size_t GetRowIds() const { return m_Records.size(); }
	int GetColCount() const { return CListCtrl_DataRecord().GetColCount(); }
	CString GetColTitle(int col) const { return CListCtrl_DataRecord().GetCellText(col, true); }

	vector<CListCtrl_DataRecord>& GetRecords() { return m_Records; }
	void SetLookupTime(int lookupTimes) { m_LookupTime = lookupTimes; }
	void SetRowMultiplier(int multiply) { if (m_RowMultiplier != multiply ) { m_RowMultiplier = multiply; InitDataModel(); } }
};
