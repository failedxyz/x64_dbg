#include "label.h"
#include "threading.h"
#include "module.h"
#include "memory.h"
#include "debugger.h"

typedef std::map<uint, LABELSINFO> LabelsInfo;

static LabelsInfo labels;

bool labelset(uint addr, const char* text, bool manual)
{
    if(!DbgIsDebugging() or !memisvalidreadptr(fdProcessInfo->hProcess, addr) or !text or strlen(text) >= MAX_LABEL_SIZE - 1 or strstr(text, "&"))
        return false;
    if(!*text) //NOTE: delete when there is no text
    {
        labeldel(addr);
        return true;
    }
    LABELSINFO label;
    label.manual = manual;
    strcpy(label.text, text);
    modnamefromaddr(addr, label.mod, true);
    label.addr = addr - modbasefromaddr(addr);
    uint key = modhashfromva(addr);
    CriticalSectionLocker locker(LockLabels);
    if(!labels.insert(std::make_pair(modhashfromva(key), label)).second) //already present
        labels[key] = label;
    return true;
}

bool labelfromstring(const char* text, uint* addr)
{
    if(!DbgIsDebugging())
        return false;
    CriticalSectionLocker locker(LockLabels);
    for(LabelsInfo::iterator i = labels.begin(); i != labels.end(); ++i)
    {
        if(!strcmp(i->second.text, text))
        {
            if(addr)
                *addr = i->second.addr + modbasefromname(i->second.mod);
            return true;
        }
    }
    return false;
}

bool labelget(uint addr, char* text)
{
    if(!DbgIsDebugging())
        return false;
    CriticalSectionLocker locker(LockLabels);
    const LabelsInfo::iterator found = labels.find(modhashfromva(addr));
    if(found == labels.end()) //not found
        return false;
    if(text)
        strcpy(text, found->second.text);
    return true;
}

bool labeldel(uint addr)
{
    if(!DbgIsDebugging())
        return false;
    CriticalSectionLocker locker(LockLabels);
    return (labels.erase(modhashfromva(addr)) > 0);
}

void labeldelrange(uint start, uint end)
{
    if(!DbgIsDebugging())
        return;
    bool bDelAll = (start == 0 && end == ~0); //0x00000000-0xFFFFFFFF
    uint modbase = modbasefromaddr(start);
    if(modbase != modbasefromaddr(end))
        return;
    start -= modbase;
    end -= modbase;
    CriticalSectionLocker locker(LockLabels);
    LabelsInfo::iterator i = labels.begin();
    while(i != labels.end())
    {
        if(i->second.manual) //ignore manual
        {
            i++;
            continue;
        }
        if(bDelAll || (i->second.addr >= start && i->second.addr < end))
            labels.erase(i++);
        else
            i++;
    }
}

void labelcachesave(JSON root)
{
    CriticalSectionLocker locker(LockLabels);
    const JSON jsonlabels = json_array();
    const JSON jsonautolabels = json_array();
    for(LabelsInfo::iterator i = labels.begin(); i != labels.end(); ++i)
    {
        const LABELSINFO curLabel = i->second;
        JSON curjsonlabel = json_object();
        json_object_set_new(curjsonlabel, "module", json_string(curLabel.mod));
        json_object_set_new(curjsonlabel, "address", json_hex(curLabel.addr));
        json_object_set_new(curjsonlabel, "text", json_string(curLabel.text));
        if(curLabel.manual)
            json_array_append_new(jsonlabels, curjsonlabel);
        else
            json_array_append_new(jsonautolabels, curjsonlabel);
    }
    if(json_array_size(jsonlabels))
        json_object_set(root, "labels", jsonlabels);
    json_decref(jsonlabels);
    if(json_array_size(jsonautolabels))
        json_object_set(root, "autolabels", jsonautolabels);
    json_decref(jsonautolabels);
}

void labelcacheload(JSON root)
{
    CriticalSectionLocker locker(LockLabels);
    labels.clear();
    const JSON jsonlabels = json_object_get(root, "labels");
    if(jsonlabels)
    {
        size_t i;
        JSON value;
        json_array_foreach(jsonlabels, i, value)
        {
            LABELSINFO curLabel;
            const char* mod = json_string_value(json_object_get(value, "module"));
            if(mod && *mod && strlen(mod) < MAX_MODULE_SIZE)
                strcpy(curLabel.mod, mod);
            else
                *curLabel.mod = '\0';
            curLabel.addr = (uint)json_hex_value(json_object_get(value, "address"));
            curLabel.manual = true;
            const char* text = json_string_value(json_object_get(value, "text"));
            if(text)
                strcpy(curLabel.text, text);
            else
                continue; //skip
            int len = (int)strlen(curLabel.text);
            for(int i = 0; i < len; i++)
                if(curLabel.text[i] == '&')
                    curLabel.text[i] = ' ';
            const uint key = modhashfromname(curLabel.mod) + curLabel.addr;
            labels.insert(std::make_pair(key, curLabel));
        }
    }
    JSON jsonautolabels = json_object_get(root, "autolabels");
    if(jsonautolabels)
    {
        size_t i;
        JSON value;
        json_array_foreach(jsonautolabels, i, value)
        {
            LABELSINFO curLabel;
            const char* mod = json_string_value(json_object_get(value, "module"));
            if(mod && *mod && strlen(mod) < MAX_MODULE_SIZE)
                strcpy(curLabel.mod, mod);
            else
                *curLabel.mod = '\0';
            curLabel.addr = (uint)json_hex_value(json_object_get(value, "address"));
            curLabel.manual = false;
            const char* text = json_string_value(json_object_get(value, "text"));
            if(text)
                strcpy_s(curLabel.text, text);
            else
                continue; //skip
            const uint key = modhashfromname(curLabel.mod) + curLabel.addr;
            labels.insert(std::make_pair(key, curLabel));
        }
    }
}

bool labelenum(LABELSINFO* labellist, size_t* cbsize)
{
    if(!DbgIsDebugging())
        return false;
    if(!labellist && !cbsize)
        return false;
    CriticalSectionLocker locker(LockLabels);
    if(!labellist && cbsize)
    {
        *cbsize = labels.size() * sizeof(LABELSINFO);
        return true;
    }
    int j = 0;
    for(LabelsInfo::iterator i = labels.begin(); i != labels.end(); ++i, j++)
    {
        labellist[j] = i->second;
        labellist[j].addr += modbasefromname(labellist[j].mod);
    }
    return true;
}

void labelclear()
{
    CriticalSectionLocker locker(LockLabels);
    LabelsInfo().swap(labels);
}