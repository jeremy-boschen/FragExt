
For a client to defragment a file, this is the sequence that takes place..
1) Client creates DefragManager instance
2) Client calls IDefragManager::QueueFile(FileName)
3) CDefragManager::QueueFile() calls CTaskManager::InsertQueueFile(FileName) via the global __pTaskManager instance
4) CTaskManager::InsertQueueFile() adds the file to the queue, signals the defrag thread, and calls CTaskDlg::_TaskQueueUpdateRoutine(QC_FILEINSERTED, FileName)
5) CTaskDlg::_TaskQueueUpdateRoutine sends a message to itself via WM_QUEUEUPDATE
6) The call stack returns to the client

When the defrag thread in CTaskManager gets its signal..
1) CTaskManager pops a file off the queue, calls IFSxFileDefragmenter::DefragmentFile(FileName)
2) IFSxFileDefragmenter::DefragmentFile() defragments the file, periodically calling CTaskManager::OnDefragmentFileUpdate()
3) CTaskManager::OnDefragmentFileUpdate() calls CTaskDlg::_TaskDefragUpdateRoutine()
4) CTaskDlg::_TaskDefragUpdateRoutine() sends a message to itself via WM_DEFRAGUPDATE

For this to work properly, CTaskDlg must be up and running before a class object is registered with COM. The
CModule::Run() method makes sure that happens.
