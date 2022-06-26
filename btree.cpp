#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <stack>

using namespace std;

class BTree {
public:
	fstream btreeFile;

	int blockSize;
	int root;
	int depth;
	int nodeNum;
	int M;
	int t;

	BTree()
	{
	};
	~BTree()
	{
		btreeFile.close();
	};

	void readHeader(const char* _fileName);
	void create(const char* _fileName, int blockSize);
	void insert(int key, int value);
	int insertLeafNode(vector<int> curBlock, int key, int value, int cur);
	int insertNonLeafNode(int parent, int key, int child);
	void print(const char* _fileName);
	int pointSearch(int key); // point search
	vector<pair<int, int>> rangeSearch(int startRange, int endRange); // range search
};

void BTree::create(const char* _fileName, int blockSize)
{
	this->btreeFile.open(_fileName, ios::binary | ios::out);

	int init = 1;

	btreeFile.write((char *)&blockSize, sizeof(int));
	btreeFile.write((char *)&init, sizeof(int));
	btreeFile.write((char *)&init, sizeof(int));
}

void BTree::readHeader(const char* _fileName)
{
	btreeFile.open(_fileName, ios::binary | ios::out | ios::in);

	btreeFile.seekg(0, ios::beg);
	btreeFile.read((char*)&blockSize, sizeof(int));
	btreeFile.read((char*)&root, sizeof(int));
	btreeFile.read((char*)&depth, sizeof(int));

	btreeFile.seekg(0, ios::end);
	this->nodeNum = btreeFile.tellg();

	nodeNum = (nodeNum - 12) / blockSize;

	M = (blockSize - 4) / 8;
	t = (M + 1) / 2 + 1;
}

int BTree::insertLeafNode(vector<int> curBlock, int key, int value, int cur)
{
	vector<pair<int, int>> records;

	for (int i = 0; i < M; i++)
	{
		int tmp1 = curBlock[2 * i];
		int tmp2 = curBlock[2 * i + 1];

		if (tmp1 || tmp2)
			records.push_back({ tmp1, tmp2 });
	}

	records.push_back({ key, value });
	sort(records.begin(), records.end());

	if (records.size() > M) //split 해야하는 경우
	{
		nodeNum++;

		vector<int> leftBlock;
		vector<int> rightBlock;

		for (int i = 0; i < M; i++) { //t-1번째까지는 현재 노드에 저장
			if (i < t) {
				leftBlock.push_back(records[i].first);
				leftBlock.push_back(records[i].second);
			}
			else {
				leftBlock.push_back(0);
				leftBlock.push_back(0);
			}
		}
		leftBlock.push_back(nodeNum);

		for (int i = t; i < t + M; i++) { //t번째부터는 다음 노드에 저장
			if (i < records.size()) {
				rightBlock.push_back(records[i].first);
				rightBlock.push_back(records[i].second);
			}
			else {
				rightBlock.push_back(0);
				rightBlock.push_back(0);
			}
		}
		rightBlock.push_back(curBlock.back());

		btreeFile.seekp(12 + (cur - 1) * blockSize, ios::beg);
		btreeFile.write((char*)&leftBlock[0], blockSize);
		btreeFile.seekp(12 + (nodeNum - 1) * blockSize, ios::beg);
		btreeFile.write((char*)&rightBlock[0], blockSize);

		return rightBlock[0];
	}
	else // split할 필요가 없는 경우
	{
		for (int i = records.size(); i < M; i++)
			records.push_back({ 0,0 });

		vector<int> newBlock;

		for (auto record : records)
		{
			newBlock.push_back(record.first);
			newBlock.push_back(record.second);
		}
		newBlock.push_back(curBlock.back());

		btreeFile.seekp(12 + (cur - 1) * blockSize, ios::beg);
		btreeFile.write((char*)&newBlock[0], blockSize);

		return -1;
	}
}

int BTree::insertNonLeafNode(int parent, int key, int child)
{
	vector<int> curBlock(blockSize / sizeof(int));

	btreeFile.seekg(12 + (parent - 1) * blockSize, ios::beg);
	btreeFile.read((char*)&curBlock[0], blockSize);

	vector<pair<int, int>> records;

	for (int i = 0; i < M; i++)
	{
		int tmp1 = curBlock[2 * i + 1];
		int tmp2 = curBlock[2 * i + 2];

		if (!tmp1 || !tmp2)
			break;
		records.push_back({ tmp1, tmp2 });
	}

	records.push_back({ key, child });
	sort(records.begin(), records.end());

	if (records.size() > M) //split 해야하는 경우
	{
		nodeNum++;

		vector<int> leftBlock;
		leftBlock.push_back(curBlock[0]);

		for (int i = 0; i < M; i++) { //t-1번째까지는 현재 노드에 저장
			if (i < t - 1) {
				leftBlock.push_back(records[i].first);
				leftBlock.push_back(records[i].second);
			}
			else {
				leftBlock.push_back(0);
				leftBlock.push_back(0);
			}
		}

		vector<int> rightBlock;
		rightBlock.push_back(records[t - 1].second);

		for (int i = t; i < t + M; i++) { //t번째부터는 다음 노드에 저장
			if (i < records.size()) {
				rightBlock.push_back(records[i].first);
				rightBlock.push_back(records[i].second);
			}
			else {
				rightBlock.push_back(0);
				rightBlock.push_back(0);
			}
		}

		btreeFile.seekp(12 + (parent - 1) * blockSize, ios::beg);
		btreeFile.write((char*)&leftBlock[0], blockSize);
		btreeFile.seekp(12 + (nodeNum - 1) * blockSize, ios::beg);
		btreeFile.write((char*)&rightBlock[0], blockSize);

		return records[t - 1].first;
	}
	else // split할 필요가 없는 경우
	{
		for (int i = records.size(); i < M; i++)
			records.push_back({ 0,0 });

		vector<int> newBlock;

		newBlock.push_back(curBlock[0]);
		for (auto record : records)
		{
			newBlock.push_back(record.first);
			newBlock.push_back(record.second);
		}

		btreeFile.seekp(12 + (parent - 1) * blockSize, ios::beg);
		btreeFile.write((char*)&newBlock[0], blockSize);

		return -1;
	}
}

void BTree::insert(int key, int value)
{
	if (nodeNum == 0) //루트가 없을 경우
	{
		vector<int> curBlock(blockSize / sizeof(int));

		curBlock[0] = key;
		curBlock[1] = value;

		btreeFile.seekp(0, ios::end);
		btreeFile.write((char*)&curBlock[0], blockSize);

		nodeNum++;
		root = 1;
	}
	else
	{
		int curDepth = 1;

		stack<int> path;
		path.push(root);

		vector<int> curBlock(blockSize / sizeof(int));

		btreeFile.seekg(12 + (root - 1) * blockSize, ios::beg);
		btreeFile.read((char*)&curBlock[0], blockSize);

		while (curDepth < depth)
		{
			int nextBlockPtr = 0;

			for (int i = 0; i < M; i++)
			{
				if (curBlock[2 * i + 1] == 0)
					break;
				if (key < curBlock[2 * i + 1])
					break;
				nextBlockPtr += 2;
			}

			path.push(curBlock[nextBlockPtr]);
			btreeFile.seekg(12 + (curBlock[nextBlockPtr] - 1) * blockSize, ios::beg);
			btreeFile.read((char*)&curBlock[0], blockSize);

			curDepth++;
		}

		int rightKey = insertLeafNode(curBlock, key, value, path.top());
		int child = nodeNum;
		path.pop();

		while (rightKey != -1)
		{
			if (path.empty())
			{
				vector<int> rootBlock;

				rootBlock.push_back(root);
				rootBlock.push_back(rightKey);
				rootBlock.push_back(nodeNum);

				for (int i = 0; i < M - 1; i++) {
					rootBlock.push_back(0);
					rootBlock.push_back(0);
				}

				nodeNum++;
				root = nodeNum;

				btreeFile.seekp(12 + (root - 1) * blockSize, ios::beg);
				btreeFile.write((char*)&rootBlock[0], blockSize);

				depth++;

				break;
			}

			rightKey = insertNonLeafNode(path.top(), rightKey, child);

			child = nodeNum;
			path.pop();
		}
	}

	btreeFile.seekp(4);
	btreeFile.write((char*)&root, sizeof(int));
	btreeFile.write((char*)&depth, sizeof(int));
}

void BTree::print(const char* _fileName)
{
	ofstream oFile(_fileName);

	int curDepth = 0;

	vector<int> curBlock(blockSize / sizeof(int));
	vector<int> nextBlockPtrs;
	nextBlockPtrs.push_back(root);

	while (curDepth < depth && curDepth < 2)
	{
		oFile << "[level " << curDepth << "]\n";

		if (curDepth + 1 == depth)
		{
			for (int j = 0; j < nextBlockPtrs.size(); j++)
			{
				btreeFile.seekg(12 + (nextBlockPtrs[j] - 1) * blockSize, ios::beg);
				btreeFile.read((char*)&curBlock[0], blockSize);

				for (int i = 0; i < M; i++)
				{
					if (curBlock[2 * i] == 0)
						break;
					if (i == 0 && j == 0)
						oFile << curBlock[2 * i];
					else
						oFile << ',' << curBlock[2 * i];
				}
			}
		}
		else
		{
			vector<int> newBLockPtrs;

			for (int j = 0; j < nextBlockPtrs.size(); j++)
			{
				btreeFile.seekg(12 + (nextBlockPtrs[j] - 1) * blockSize, ios::beg);
				btreeFile.read((char*)&curBlock[0], blockSize);

				for (int i = 0; i < M; i++)
				{
					if (curBlock[2 * i + 1] == 0)
						break;

					if (i == 0 && j == 0)
						oFile << curBlock[2 * i + 1];
					else
						oFile << ',' << curBlock[2 * i + 1];
				}

				for (int i = 0; i <= M; i++)
				{
					if (curBlock[2 * i] == 0)
						break;

					newBLockPtrs.push_back(curBlock[2 * i]);
				}
			}

			nextBlockPtrs.clear();
			
			for (int newBLockPtr : newBLockPtrs)
				nextBlockPtrs.push_back(newBLockPtr);

			newBLockPtrs.clear();
		}
		oFile << "\n\n";
		curDepth++;
	}
}

int BTree::pointSearch(int key)
{
	int curDepth = 1;

	vector<int> curBlock(blockSize / sizeof(int));

	btreeFile.seekg(12 + (root - 1) * blockSize, ios::beg);
	btreeFile.read((char*)&curBlock[0], blockSize);
	
	while (curDepth < depth)
	{
		int nextBlockPtr = 0;

		for (int i = 0; i < M; i++)
		{
			if (curBlock[2 * i + 1] == 0)
				break;
			if (key < curBlock[2 * i + 1])
				break;
			nextBlockPtr += 2;
		}

		btreeFile.seekg(12 + (curBlock[nextBlockPtr] - 1) * blockSize, ios::beg);
		btreeFile.read((char*)&curBlock[0], blockSize);

		curDepth++;
	}

	for (int i = 0; i < M; i++)
	{
		if (key == curBlock[2 * i])
			return curBlock[2 * i + 1];
	}

	return -1;
}

vector<pair<int,int>> BTree::rangeSearch(int startRange, int endRange)
{
	vector<pair<int, int>> res;

	int curDepth = 1;
	int nextBlockPtr = 0;

	vector<int> curBlock(blockSize / sizeof(int));

	btreeFile.seekg(12 + (root - 1) * blockSize, ios::beg);
	btreeFile.read((char*)&curBlock[0], blockSize);

	while (curDepth < depth)
	{
		nextBlockPtr = 0;

		for (int i = 0; i < M; i++)
		{
			if (curBlock[2 * i + 1] == 0)
				break;
			if (startRange < curBlock[2 * i + 1])
				break;
			nextBlockPtr += 2;
		}

		btreeFile.seekg(12 + (curBlock[nextBlockPtr] - 1) * blockSize, ios::beg);
		btreeFile.read((char*)&curBlock[0], blockSize);

		curDepth++;
	}

	nextBlockPtr = curBlock.back();

	while (true)
	{
		for (int i = 0; i < M; i++)
		{
			if (curBlock[2 * i] == 0)
			{
				nextBlockPtr = curBlock.back();
				break;
			}

			if (startRange > curBlock[2 * i])
				continue;
			else if (curBlock[2 * i] > endRange)
			{
				nextBlockPtr = 0;
				break;
			}
			else
				res.push_back({ curBlock[2 * i] , curBlock[2 * i + 1] });
		}

		if (nextBlockPtr == 0)
			break;

		btreeFile.seekg(12 + (curBlock.back() - 1) * blockSize, ios::beg);
		btreeFile.read((char*)&curBlock[0], blockSize);
	}

	return res;
}

// Test
int main(int argc, char* argv[])
{

	char command = argv[1][0];
	ofstream outFile;
	ifstream inFile;

	BTree* myBtree = new BTree();

	switch (command)
	{
	case 'c':
		myBtree->create(argv[2], stoi(argv[3]));
		break;
	case 'i':
	{
		myBtree->readHeader(argv[2]);

		inFile.open(argv[3]);

		if (inFile.fail())
		{
			cerr << "해당 insert 파일을 찾을 수 없습니다.\n";

			return -1;
		}

		string keyStr;
		string idStr;

		while (getline(inFile, keyStr, ','))
		{
			if (!getline(inFile, idStr))
			{
				cerr << "insert 파일의 형식이 잘못되었습니다.\n";
				return -1;
			}

			if (!isdigit(keyStr[0]) || !isdigit(idStr[0]))
				continue;

			int key = stoi(keyStr);
			int id = stoi(idStr);

			myBtree->insert(key, id);
		}
		
		inFile.close();
		break;
	}
	case 's':
	{
		myBtree->readHeader(argv[2]);

		inFile.open(argv[3]);
		outFile.open(argv[4]);

		if (inFile.fail())
		{
			cerr << "해당 search 파일을 찾을 수 없습니다.\n";

			return -1;
		}

		string keyStr;

		while (getline(inFile, keyStr))
		{
			if (!isdigit(keyStr[0]))
				continue;
			int key = stoi(keyStr);

			int value = myBtree->pointSearch(key);

			outFile << key << "," << value << "\n";
		}

		inFile.close();
		outFile.close();
		break;
	}
	case 'r':
	{
		myBtree->readHeader(argv[2]);

		inFile.open(argv[3]);
		outFile.open(argv[4]);

		if (inFile.fail())
		{
			cerr << "해당 search 파일을 찾을 수 없습니다.\n";

			return -1;
		}

		string startRangeStr;
		string endRangeStr;

		while (getline(inFile, startRangeStr, ','))
		{
			if (!getline(inFile, endRangeStr))
			{
				cerr << "insert 파일의 형식이 잘못되었습니다.\n";
				return -1;
			}

			if (!isdigit(startRangeStr[0]) || !isdigit(endRangeStr[0]))
				continue;

			int startRange = stoi(startRangeStr);
			int endRange = stoi(endRangeStr);

			vector<pair<int, int>> rangeRecords = myBtree->rangeSearch(startRange, endRange);

			for (auto record : rangeRecords)
				outFile << record.first << "," << record.second << " / ";
			outFile << "\n";
		}

		inFile.close();
		outFile.close();
		break;
	}
	case 'p':
		myBtree->readHeader(argv[2]);
		myBtree->print(argv[3]);
		myBtree->btreeFile.close();
		break;
	}

	return 0;
}