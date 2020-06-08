# Sitare Arslantürk, 57677
# Comp 304: Project 3
import random
import datetime


class Directory:
    def __init__(self):  # Constructor of Directory
        self.fixed_block_list_size = 32768
        self.directory_content = [0] * self.fixed_block_list_size  # DC
        self.directory_table = dict()  # DT

    def __repr__(self):  # Overrides built-in repr method
        return "Directory is composed of directory content and DT "

    def changeSize(self, block_size):  # calculates the remaining space for DC after giving space to FAT
        rest = self.fixed_block_list_size * block_size // (4 + block_size)  # 4 bytes is given
        self.fixed_block_list_size = rest
        self.directory_content = [0] * self.fixed_block_list_size  # directory content's size is updated


class FAT:
    def __init__(self):  # Constructor of FAT
        # index will point to the next block
        self.fat_size = 32768
        self.fat_dict = [-9] * self.fat_size

    def __repr__(self):  # Overrides built-in repr method
        print("Fixed block size of " + str(int(len(self.fat_dict))))
        return "Directory is composed of directory content and DT "


class Info:  # object used to hold length and start index in dictionary
    def __init__(self, length, startIndex):
        self.length = length
        self.index = startIndex


class FileSystem(object):  # parent class of linked and contiguous implementation
    def __init__(self, fixed_block_size):
        self.fixed_block_size = fixed_block_size
        self.directory = Directory()
        self.rejectionCounterforCreate = 0
        self.rejectionCounterforExtend = 0

    def __repr__(self):
        return "File System Object"

    def changeSize(self):  # method to be used in linked implementation due to FAT usage
        self.directory.changeSize(self.fixed_block_size)


class Contiguous(FileSystem):
    def __init__(self, fixed_block_size):
        FileSystem.__init__(self, fixed_block_size)

    def __repr__(self):
        return "Contiguous Implementation"

    # create_file method:
    # allocates space for the file of size file_length on the disk
    # updates the directory table
    def create_file(self, file_id, file_length):
        # get the starting and ending index
        length = self.getEndIndex(file_length)
        flag, startIndex = self.isSpaceAvailable(0, length)  # check if space is available
        if flag:
            # check if space is contiguous
            if self.isSpaceContiguous(startIndex, length):
                self.allocate(startIndex, length, file_id)  # if enough contiguous space, allocate

            else:  # else defragment and allocate
                self.defragment()  # method explained
                startIndex = self.getStartIndex()
                self.allocate(startIndex, length, file_id)
        else:
            self.rejectionCounterforCreate += 1  # increase counter if rejected
            print("Rejection for " + str(file_id) + " with length " + str(file_length))

    # helper method: isSpaceAvailable
    # checks if total space needed is available
    def isSpaceAvailable(self, start, length):
        counter = 0
        for i in range(start, len(self.directory.directory_content)):
            if self.directory.directory_content[i] == 0:
                counter += 1
                if counter == 1:
                    startIndex = i
                if counter == length:
                    return True, startIndex  # returns True and the starting index of the space
        return False, None  # returns False and nothing for the starting index

    # helper method: defragment
    # defragmentation pushes all the filled elements of the directory content to the left without changing their order
    # then the directory table is updated with the new order
    def defragment(self):
        initial_free = -1
        i = 0
        while i < self.directory.fixed_block_list_size:  # traverses to find the first empty space
            if self.directory.directory_content[i] == 0:  # if there is empty space
                if initial_free == -1:
                    initial_free = i  # initial free space index is set to i

            if initial_free != -1 and self.directory.directory_content[i] != 0:  # when finds the initial empty space
                self.directory.directory_content[initial_free], self.directory.directory_content[i] = \
                    self.directory.directory_content[i], self.directory.directory_content[initial_free]
                self.updateTable(i, initial_free)  # swaps the contents and updates the table with the new positions
                i = initial_free
                initial_free = -1
            i += 1

    # helper method: updateTable
    # used when an update in the Directory Table is needed
    def updateTable(self, j, index):
        key_list = list(self.directory.directory_table.keys())
        val_list = list(self.directory.directory_table.values())
        for i in range(len(val_list)):
            val = val_list[i]
            key = key_list[i]
            if val.index == j:
                new_info = Info(val.length, index)
                self.directory.directory_table.update({key: new_info})

    # helper method: getStartIndex
    # used to get the first empty index in the directory content
    def getStartIndex(self):
        for i in range(self.directory.fixed_block_list_size):
            if self.directory.directory_content[i] == 0:
                return i

    # helper method: getEndIndex
    # used to get the number of blocks needed to allocate the file of file_length bytes
    # returns the number of blocks or otherwise the ending index of the file since it is contiguous
    def getEndIndex(self, file_length):
        if file_length > self.fixed_block_size:
            length = file_length // self.fixed_block_size
            if file_length % self.fixed_block_size != 0:
                length += 1
        elif file_length == self.fixed_block_size:
            length = 1
        else:
            length = 1
        return length

    # helper method: allocate
    # allocates a random integer to the directory content in order to show the index is full
    def allocate(self, startIndex, length, file_id):
        for i in range(startIndex, startIndex + length):
            self.directory.directory_content[i] = random.randint(1, self.fixed_block_size)
        info = Info(length, startIndex)
        self.directory.directory_table.update({str(file_id): info})

    # helper method: isSpaceContiguous
    # checks if space available is free in a contiguous way
    # returns True if satisfied
    def isSpaceContiguous(self, startIndex, length):
        final = startIndex + length
        for i in range(startIndex, final):
            if self.directory.directory_content[i] != 0:
                return False
        return True

    # access method: returns the location of the byte given the byte-offset
    def access(self, file_id, byte_offset):
        val = self.directory.directory_table.get(str(file_id))
        if val is not None:
            startIndex = val.index
            file_length = val.length
            endIndex = self.getEndIndex(byte_offset)  # receives the corresponding block number when startIndex = 0
            returnIndex = startIndex + endIndex  # add the number of blocks to the starting index
            if returnIndex > startIndex + file_length:
                return -1
        else:
            returnIndex = -1
        return returnIndex

    # extend method: extends the given file by the number of extension blocks
    # checks if there is available space first
    # is space is available,
    # checks if space is contiguous, if yes then allocates
    # else if space is not contiguous, it defragments
    # defragmenting means pushing all the filled elements to the left
    # then in order to add the extension to the right of the file
    # composition is performed which is pushing all the files right to the ending index of that file
    # number of units right, so that there is enough space available for contiguous extension
    # updates the directory table
    def extend(self, file_id, extension):
        val = self.directory.directory_table.get(str(file_id))
        if val is not None:
            startIndex = val.index
            length = val.length
            lastIndex = startIndex + length - 1
            spaceCheck = self.isSpaceAvailable(0, extension)[0]  # total space check
            if spaceCheck:

                if self.isSpaceContiguous(lastIndex, extension + 1):  # contiguous space check
                    # allocate
                    for i in range(lastIndex, lastIndex + extension + 1):
                        self.directory.directory_content[i] = random.randint(1, self.fixed_block_size)
                        updated_info = Info(length + extension, startIndex)
                        self.directory.directory_table.update({str(file_id): updated_info})
                else:
                    self.defragment()  # defragment : push left
                    self.composition(extension, lastIndex)  # composition: push right the amount of extension
                    startIndex = self.directory.directory_table.get(str(file_id)).index
                    self.allocate(startIndex + length, extension, file_id)  # allocate extension to our file's end
            else:  # If there is no sufficient space to extend the file, reject
                self.rejectionCounterforExtend += 1
                print("Rejecting to extend")
        else:
            self.rejectionCounterforExtend += 1
            print("Extend Error: File Id could not be located in the directory table")

    # helper method: composition
    # after defragmenting, the extension needs to be put to the end of the file
    # so all the files that are at our files right are pushed right by the amount of extension
    # in the new opened space, the extension could be allocated
    def composition(self, extension, lastIndex):
        lastBlock = -1
        for i in reversed(range(0, self.directory.fixed_block_list_size)):
            if self.directory.directory_content[i] != 0:  # traverses to find the last Block
                lastBlock = i
                break

        while lastBlock > lastIndex:  # moves the content by the amount of extension to the right
            self.directory.directory_content[lastBlock + extension] = self.directory.directory_content[lastBlock]
            self.updateTable(lastBlock, lastBlock + extension)
            lastBlock -= 1

        for j in range(lastIndex + 1, lastIndex + extension + 1):
            self.directory.directory_content[j] = 0  # the emptied space is filled with 0 for the extension to come

    # helper method: removeFrom
    # used in shrink in order to remove the content's random integer and make it 0
    # to show that the file has been shrinked
    def removeFrom(self, startIndex, lastIndex):
        for i in reversed(range(startIndex, lastIndex)):
            self.directory.directory_content[i] = 0

    # shrink method
    # shrinks the file by the given number of blocks
    # if shrinking amount is larger than the shrinking is not performed
    # updates the directory table
    def shrink(self, file_id, shrinking):
        val = self.directory.directory_table.get(str(file_id))
        if val is not None:
            startIndex = val.index
            length = val.length
            # does not perform shrink if shrinking block size is bigger than or equal to length
            # because there should be at least 1 block of the file remaining
            if shrinking >= length:
                return
            lastIndex = startIndex + length
            self.removeFrom(lastIndex - shrinking, lastIndex)
            updated_info = Info(length - shrinking, startIndex)  # updates the directory table
            self.directory.directory_table.update({str(file_id): updated_info})
        else:
            print("Shrink Error: File Id could not be located in the directory table")


class Linked(FileSystem):
    def __init__(self, fixed_block_size):
        FileSystem.__init__(self, fixed_block_size)
        self.fat = FAT()  # FAT initialized
        FileSystem.changeSize(self)  # space allocated for directory content is reduced due to FAT

    def __repr__(self):
        return "Linked Implementation"

    # create_file method:
    # allocates space for the file of size file_length on the disk
    # updates the directory table and FAT
    def create_file(self, file_id, file_length):
        # get the starting and ending index
        length = self.getEndIndex(file_length)
        flag, startIndex = self.isSpaceAvailable(0, length)

        # check if space is available
        if flag:
            prev_index = -10
            i = 0
            while i < length:
                index = self.findEmptySpace()
                if index != -1:
                    # end of file points to -1

                    # update the FAT
                    if prev_index != -10:
                        self.fat.fat_dict[prev_index] = index
                        prev_index = -10

                    # check if space is empty, allocate
                    self.directory.directory_content[index] = random.randint(1, self.fixed_block_size)

                    # start holding the prev index
                    if prev_index == -10:
                        prev_index = index

                    if i == length - 1:
                        self.fat.fat_dict[prev_index] = index
                        self.directory.directory_content[index] = random.randint(1, self.fixed_block_size)
                        self.fat.fat_dict[index] = -1

                        info = Info(length, startIndex)
                        self.directory.directory_table.update({str(file_id): info})
                        print("created file id: " + str(file_id))
                        return
                i += 1
        else:
            self.rejectionCounterforCreate += 1
            print("Rejection for " + str(file_id) + " with length " + str(file_length))
            print("NO SPACE AVAILABLE FOR CREATE CALL")

    # helper method: isSpaceAvailable
    # checks if total space needed is available
    def isSpaceAvailable(self, start, length):
        counter = 0
        for i in range(start, len(self.directory.directory_content)):
            if self.directory.directory_content[i] == 0:
                counter += 1
                if counter == 1:
                    startIndex = i
                if counter == length:
                    return True, startIndex
        return False, None

    # helper method: getEndIndex
    # used to get the number of blocks needed to allocate the file of file_length bytes
    # returns the number of blocks or otherwise the ending index of the file since it is contiguous
    def getEndIndex(self, file_length):
        if file_length > self.fixed_block_size:
            length = file_length // self.fixed_block_size
            if file_length % self.fixed_block_size != 0:
                length += 1
        elif file_length == self.fixed_block_size:
            length = 1
        else:
            length = 1
        return length

    # helper method: findEmptySpace
    # finds the first empty space in the directory content
    def findEmptySpace(self):
        for i in range(0, self.directory.fixed_block_list_size):
            if self.directory.directory_content[i] == 0:
                return i
        return -1

    # access method:
    # traverses the FAT
    # to get to the number of the block corresponding to the byte-offset
    # returns that specific index
    def access(self, file_id, byte_offset):
        val = self.directory.directory_table.get(str(file_id))
        if val is not None:
            startIndex = val.index
            initialIndex = startIndex
            file_length = val.length
            endIndex = self.getEndIndex(byte_offset)
            totalCheck = startIndex + endIndex
            i = 0
            while i < endIndex - 1:
                initial = self.fat.fat_dict[startIndex]
                startIndex = initial
                i += 1
            returnIndex = startIndex

            if totalCheck > file_length + initialIndex:
                return -1
        else:
            return -1
        return returnIndex

    # helper method: getPrevLastIndexForFAT
    # returns the file's index at FAT that holds -1 to show the end of file
    def getPrevLastIndexForFAT(self, startIndex, length):
        # traverse FAT to get last index
        for i in range(0, length - 1):
            initial = self.fat.fat_dict[startIndex]
            startIndex = initial
        return startIndex

    # extend method: extends the file by the given number of block extensions, details explained below
    # allocates space in the directory content
    # updates the directory table and FAT
    def extend(self, file_id, extension):
        val = self.directory.directory_table.get(str(file_id))
        if val is not None:
            startIndex = val.index
            length = val.length
            # If there is no sufficient space to extend the file, reject
            spaceCheck = self.isSpaceAvailable(0, extension)[0]
            if spaceCheck:
                prev_index = -10
                i = 0
                prevLastIndex = self.getPrevLastIndexForFAT(startIndex, length)

                while i < extension:
                    index = self.findEmptySpace()
                    if index != -1:
                        # end of file points to -1
                        if i == 0:
                            self.fat.fat_dict[prevLastIndex] = index

                        # update the FAT
                        if prev_index != -10:
                            self.fat.fat_dict[prev_index] = index
                            prev_index = -10

                        # allocate to directory content
                        self.directory.directory_content[index] = random.randint(1, self.fixed_block_size)

                        # start holding the prev index
                        if prev_index == -10:
                            prev_index = index

                        if i == extension - 1:
                            # if the last index of the file is reached, need to indicate -1
                            self.fat.fat_dict[prev_index] = index
                            self.directory.directory_content[index] = random.randint(1, self.fixed_block_size)
                            self.fat.fat_dict[index] = -1
                            # update the directory table
                            updated_info = Info(length + extension, startIndex)
                            self.directory.directory_table.update({str(file_id): updated_info})
                            return

                    i += 1

            else:
                self.rejectionCounterforExtend += 1
                print("Rejecting to extend")
        else:
            self.rejectionCounterforExtend += 1
            print("Extend Error: File Id could not be located in the directory table")

    # shrink method
    # shrinks the file by the given number of blocks
    # if shrinking amount is larger than the shrinking is not performed
    # removes shrink space from the directory content
    # updates the directory table and FAT
    def shrink(self, file_id, shrinking):
        val = self.directory.directory_table.get(str(file_id))
        if val is not None:
            startIndex = val.index
            length = val.length
            # does not perform shrink if shrinking block size is bigger than or equal to length
            # because there should be at least 1 block of the file remaining
            if shrinking >= length:
                return
            # has to traverse the FAT in order to find the last index and start shrinking from file's end
            for i in range(0, shrinking):
                prevLastIndex = self.getPrevLastIndexForFAT(startIndex, length - i)
                self.directory.directory_content[prevLastIndex] = 0
                self.fat.fat_dict[prevLastIndex] = -9
            # to update the last index of the file in FAT so that it indicates -1
            prevLastIndex = self.getPrevLastIndexForFAT(startIndex, length - shrinking)
            self.fat.fat_dict[prevLastIndex] = -1
            # updates the directory table
            updated_info = Info(length - shrinking, startIndex)
            self.directory.directory_table.update({str(file_id): updated_info})
        else:
            print("Shrink Error: File Id could not be located in the directory table")

    # helper method: removeFrom
    # used in shrink in order to remove the content's random integer and make it 0
    # to show that the file has been shrinked
    def removeFrom(self, startIndex, lastIndex):
        for i in reversed(range(startIndex, lastIndex)):
            self.directory.directory_content[i] = 0


def main():
    total_access_time = 0
    total_create_time = 0
    total_extend_time = 0
    total_shrink_time = 0

    total_create_operations = 0
    total_shrink_operations = 0
    total_extend_operations = 0
    total_access_operations = 0

    textInput = "input_1024_200_9_0_9.txt"

    split = textInput.split('_')
    block_size = int(split[1])
    idCounter = 0

    # decide on type of file system implementation here
    fileSystem = Linked(block_size)
    # fileSystem = Contiguous(block_size)

    with open(textInput) as fp:
        Lines = fp.readlines()
        for line in Lines:
            line_split = line.split(':')
            if line_split[0] == "a":
                file_id = int(line_split[1])
                offset = int(line_split[2].strip())
                print("Access Call for File_id: " + line_split[1] + ", Offset: " + str(offset))
                access_start_time = datetime.datetime.now()
                returnIndex = fileSystem.access(file_id, offset)
                access_finish_time = datetime.datetime.now()
                at = access_finish_time - access_start_time
                total_access_time += at.microseconds
                total_access_operations += 1

            elif line_split[0] == "c":
                file_length = line_split[1].strip()
                print("Create call for Bytes: " + file_length)
                create_start_time = datetime.datetime.now()
                fileSystem.create_file(idCounter, int(file_length))
                create_finish_time = datetime.datetime.now()
                idCounter += 1
                ct = create_finish_time - create_start_time
                total_create_time += ct.microseconds
                total_create_operations += 1

            elif line_split[0] == "e":
                file_id = int(line_split[1])
                extension = int(line_split[2].strip())
                print("Extend call for File_id: " + str(file_id) + ", Extension: " + str(extension))
                extend_start_time = datetime.datetime.now()
                fileSystem.extend(file_id, extension)
                extend_finish_time = datetime.datetime.now()
                et = extend_finish_time - extend_start_time
                total_extend_time += et.microseconds
                total_extend_operations += 1

            elif line_split[0] == "sh":
                file_id = int(line_split[1])
                shrinking = int(line_split[2].strip())
                print("Shrink call for File_id: " + str(file_id) + ", Shrinking: " + str(shrinking))
                shrink_start_time = datetime.datetime.now()
                fileSystem.shrink(file_id, shrinking)
                shrink_finish_time = datetime.datetime.now()
                st = shrink_finish_time - shrink_start_time
                total_shrink_time += st.microseconds
                total_shrink_operations += 1

            else:
                print("---------- unknown call, check text file ------------")

        if total_create_operations == 0:
            time_per_create = 0
        else:
            time_per_create = total_create_time / total_create_operations

        if total_access_operations == 0:
            time_per_access = 0
        else:
            time_per_access = total_access_time / total_access_operations

        if total_extend_operations == 0:
            time_per_extend = 0
        else:
            time_per_extend = total_extend_time / total_extend_operations

        if total_shrink_operations == 0:
            time_per_shrink = 0
        else:
            time_per_shrink = total_shrink_time / total_shrink_operations

        total_elapsed_time = total_create_time + total_extend_time + total_shrink_time + total_access_time

        print("\n----------------------------- Summary -----------------------------------")
        print("\nTotal rejection for create call: " + str(fileSystem.rejectionCounterforCreate))
        print("Total rejection for extend call: " + str(fileSystem.rejectionCounterforExtend))
        print("\n-------------------------------------------------------------------------")
        print("Average Time per Access Operation: " + str(time_per_access) + " microseconds")
        print("Average Time per Create Operation: " + str(time_per_create) + " microseconds")
        print("Average Time per Extend Operation: " + str(time_per_extend) + " microseconds")
        print("Average Time per Shrink Operation: " + str(time_per_shrink) + " microseconds")


if __name__ == "__main__":
    main()
