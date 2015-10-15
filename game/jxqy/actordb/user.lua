module('Actordb', package.seeall)

Task = {
    [1] = 'int uid',
    [2] = 'int taskid',
}


User = {
    [1] = 'int uid',
    [2] = 'String* uname',
    [3] = 'Array<Task>* task_array',
    [4] = 'Table<Task>* task_table',
}


return User
