#include <iostream>
#include <iosfwd>
#include <string>
#include <ctime>

#include <stdio.h>
#include "rstartree.h"
#include "profile.h"

RStarBoundingBox<2> createbox(int x, int y, int w, int h) {
    RStarBoundingBox<2> box;
    box.min_edges[0] = x;
    box.min_edges[1] = y;
    box.max_edges[0] = x + w;
    box.max_edges[1] = y + h;
    return box;
}


int main() {
    using namespace std;


    int quality = 0;
    for (size_t i = 0; i < 30; i++)
    {
        RStarTree<int, 2, 50, 100> tree;
        int nodes = 20000;
        vector<pair<int, RStarBoundingBox<2>>> vec;
        vector<pair<int, RStarBoundingBox<2>>> bruteforce_res;
        srand(std::time(0));

        for (int i = 0; i < nodes / 2; i++) {
            auto box = createbox(rand() % 1000, rand() % 1000, rand() % 10, rand() % 10);
            tree.insert(i, box);
            vec.push_back({ i,box });
        }


        for (int i = 0; i < nodes / 2; i++) {
            auto box = createbox(rand() % 1000, rand() % 1000, rand() % 20, rand() % 20);
            tree.insert(i, box);
            vec.push_back({ i,box });
        }

        auto areafind = createbox(rand() % 1000, rand() % 1000, rand() % 40, rand() % 40);
        auto structure_res = tree.find_objects_in_area(areafind);
        for (const auto& w : vec) {
            if (w.second.is_intersected(areafind)) {
                bruteforce_res.push_back(w);
            }
        }
        sort(structure_res.begin(), structure_res.end());
        sort(bruteforce_res.begin(), bruteforce_res.end());
        bool flag(true);
        if (structure_res.size() == bruteforce_res.size()) {
            for (size_t i = 0; i < bruteforce_res.size(); i++) {
                if (structure_res[i].get_box() != bruteforce_res[i].second
                    || structure_res[i].get_value() != bruteforce_res[i].first) {
                    flag = false;
                }
            }
        }
        else {
            flag = false;
        }
        if (flag) {
            quality++;
        }
    }
    cout << "Correctness of results is " << quality << "/30";

    cout << endl << endl;
    {
        RStarTree<int, 2, 50, 100> tree;
        int nodes = 1000;
        LOG_DURATION("Insertion of 1000 of objects")
            for (int i = 0; i < nodes / 2; i++) {
                auto box = createbox(rand() % 1000, rand() % 1000, rand() % 10, rand() % 10);
                tree.insert(i, box);
            }


        for (int i = 0; i < nodes / 2; i++) {
            auto box = createbox(rand() % 1000, rand() % 1000, rand() % 20, rand() % 20);
            tree.insert(i, box);
        }
    }
    {
        RStarTree<int, 2, 50, 100> tree;
        int nodes = 5000;
        LOG_DURATION("Insertion of 5000 of objects")
            for (int i = 0; i < nodes / 2; i++) {
                auto box = createbox(rand() % 1000, rand() % 1000, rand() % 10, rand() % 10);
                tree.insert(i, box);
            }


        for (int i = 0; i < nodes / 2; i++) {
            auto box = createbox(rand() % 1000, rand() % 1000, rand() % 20, rand() % 20);
            tree.insert(i, box);
        }
    }

    {
        RStarTree<int, 2, 50, 100> tree;
        int nodes = 10000;
        LOG_DURATION("Insertion of 10000 of objects")
            for (int i = 0; i < nodes / 2; i++) {
                auto box = createbox(rand() % 1000, rand() % 1000, rand() % 10, rand() % 10);
                tree.insert(i, box);
            }


        for (int i = 0; i < nodes / 2; i++) {
            auto box = createbox(rand() % 1000, rand() % 1000, rand() % 20, rand() % 20);
            tree.insert(i, box);
        }
    }

    {
        RStarTree<int, 2, 50, 100> tree;
        int nodes = 50000;
        LOG_DURATION("Insertion of 50000 of objects")
            for (int i = 0; i < nodes / 2; i++) {
                auto box = createbox(rand() % 1000, rand() % 1000, rand() % 10, rand() % 10);
                tree.insert(i, box);
            }


        for (int i = 0; i < nodes / 2; i++) {
            auto box = createbox(rand() % 1000, rand() % 1000, rand() % 20, rand() % 20);
            tree.insert(i, box);
        }
    }

    {
        RStarTree<int, 2, 50, 100> tree;
        int nodes = 100000;
        LOG_DURATION("Insertion of 100000 of objects");
        for (int i = 0; i < nodes / 2; i++) {
            auto box = createbox(rand() % 1000, rand() % 1000, rand() % 10, rand() % 10);
            tree.insert(i, box);
        }

        for (int i = 0; i < nodes / 2; i++) {
            auto box = createbox(rand() % 1000, rand() % 1000, rand() % 20, rand() % 20);
            tree.insert(i, box);
        }
    }
    cout << endl;
    {
        RStarTree<int, 2, 50, 100> tree;
        int nodes = 1000;
        for (int i = 0; i < nodes / 2; i++) {
            auto box = createbox(rand() % 1000, rand() % 1000, rand() % 10, rand() % 10);
            tree.insert(i, box);
        }

        for (int i = 0; i < nodes / 2; i++) {
            auto box = createbox(rand() % 1000, rand() % 1000, rand() % 20, rand() % 20);
            tree.insert(i, box);
        }
        {
            int deleted_nodes = 10000;
            LOG_DURATION("Deletion of 1000 nodes");
            for (int i = 0; i < deleted_nodes; i++) {
                auto box = createbox(rand() % 1000, rand() % 1000, rand() % 20, rand() % 20);
                tree.delete_objects_in_area(box);
            }
        }
    }
    {
        RStarTree<int, 2, 50, 100> tree;
        int nodes = 10000;
        for (int i = 0; i < nodes / 2; i++) {
            auto box = createbox(rand() % 1000, rand() % 1000, rand() % 10, rand() % 10);
            tree.insert(i, box);
        }

        for (int i = 0; i < nodes / 2; i++) {
            auto box = createbox(rand() % 1000, rand() % 1000, rand() % 20, rand() % 20);
            tree.insert(i, box);
        }
        {
            int deleted_nodes = 10000;
            LOG_DURATION("Deletion of 10000 nodes");
            for (int i = 0; i < deleted_nodes; i++) {
                auto box = createbox(rand() % 1000, rand() % 1000, rand() % 20, rand() % 20);
                tree.delete_objects_in_area(box);
            }
        }
    }
    {
        RStarTree<int, 2, 50, 100> tree;
        int nodes = 100000;
        for (int i = 0; i < nodes / 2; i++) {
            auto box = createbox(rand() % 1000, rand() % 1000, rand() % 10, rand() % 10);
            tree.insert(i, box);
        }

        for (int i = 0; i < nodes / 2; i++) {
            auto box = createbox(rand() % 1000, rand() % 1000, rand() % 20, rand() % 20);
            tree.insert(i, box);
        }
        {
            int deleted_nodes = 10000;
            LOG_DURATION("Deletion of 100000 nodes");
            for (int i = 0; i < deleted_nodes; i++) {
                auto box = createbox(rand() % 1000, rand() % 1000, rand() % 20, rand() % 20);
                tree.delete_objects_in_area(box);
            }
        }
    }
    {
        RStarTree<int, 2, 15, 32> tree;
        int nodes = 1000;
        vector<pair<int, RStarBoundingBox<2>>> vec;



        for (int i = 0; i < nodes / 2; i++) {
            auto box = createbox(rand() % 1000, rand() % 1000, rand() % 10, rand() % 10);
            tree.insert(i, box);
            vec.push_back({ i,box });
        }

        for (int i = 0; i < nodes / 2; i++) {
            auto box = createbox(rand() % 1000, rand() % 1000, rand() % 5, rand() % 5);
            tree.insert(i, box);
            vec.push_back({ i,box });
        }

        cout << endl;

        {
            LOG_DURATION("Time of searching with R-star-tree with 1000 objects");
            for (size_t i = 0; i < 100000; i++) {
                auto box = createbox(rand() % 1000, rand() % 1000, rand() % 10, rand() % 10);
                auto structure_res = tree.find_objects_in_area(box);
            }

        }

        {
            LOG_DURATION("Time of searching with brute-force 1000 objects");
            for (size_t i = 0; i < 100000; i++) {
                auto box = createbox(rand() % 1000, rand() % 1000, rand() % 10, rand() % 10);
                vector<pair<int, RStarBoundingBox<2>>> bruteforce_res;
                for (const auto& w : vec) {
                    if (w.second.is_intersected(box)) {
                        bruteforce_res.push_back(w);
                    }
                }
            }

        }

    }
    {
        RStarTree<int, 2, 15, 32> tree;
        int nodes = 5000;
        vector<pair<int, RStarBoundingBox<2>>> vec;



        for (int i = 0; i < nodes / 2; i++) {
            auto box = createbox(rand() % 1000, rand() % 1000, rand() % 10, rand() % 10);
            tree.insert(i, box);
            vec.push_back({ i,box });
        }

        for (int i = 0; i < nodes / 2; i++) {
            auto box = createbox(rand() % 1000, rand() % 1000, rand() % 5, rand() % 5);
            tree.insert(i, box);
            vec.push_back({ i,box });
        }

        cout << endl;

        {
            LOG_DURATION("Time of searching with R-star-tree with 5000 objects");
            for (size_t i = 0; i < 100000; i++) {
                auto box = createbox(rand() % 1000, rand() % 1000, rand() % 10, rand() % 10);
                auto structure_res = tree.find_objects_in_area(box);
            }

        }

        {
            LOG_DURATION("Time of searching with brute-force 5000 objects");
            for (size_t i = 0; i < 100000; i++) {
                auto box = createbox(rand() % 1000, rand() % 1000, rand() % 10, rand() % 10);
                vector<pair<int, RStarBoundingBox<2>>> bruteforce_res;
                for (const auto& w : vec) {
                    if (w.second.is_intersected(box)) {
                        bruteforce_res.push_back(w);
                    }
                }
            }

        }

    }

    {
        RStarTree<int, 2, 15, 32> tree;
        int nodes = 10000;
        vector<pair<int, RStarBoundingBox<2>>> vec;



        for (int i = 0; i < nodes / 2; i++) {
            auto box = createbox(rand() % 1000, rand() % 1000, rand() % 10, rand() % 10);
            tree.insert(i, box);
            vec.push_back({ i,box });
        }

        for (int i = 0; i < nodes / 2; i++) {
            auto box = createbox(rand() % 1000, rand() % 1000, rand() % 5, rand() % 5);
            tree.insert(i, box);
            vec.push_back({ i,box });
        }

        cout << endl;

        {
            LOG_DURATION("Time of searching with R-star-tree with 10000 objects");
            for (size_t i = 0; i < 100000; i++) {
                auto box = createbox(rand() % 1000, rand() % 1000, rand() % 10, rand() % 10);
                auto structure_res = tree.find_objects_in_area(box);
            }

        }

        {
            LOG_DURATION("Time of searching with brute-force 10000 objects");
            for (size_t i = 0; i < 100000; i++) {
                auto box = createbox(rand() % 1000, rand() % 1000, rand() % 10, rand() % 10);
                vector<pair<int, RStarBoundingBox<2>>> bruteforce_res;
                for (const auto& w : vec) {
                    if (w.second.is_intersected(box)) {
                        bruteforce_res.push_back(w);
                    }
                }
            }

        }

    }
    {
        RStarTree<int, 2, 6, 12> tree;
        RStarTree<int, 2, 6, 12> treefromfile;
        for (int i = 0; i < 10000; i++) {
            auto box = createbox(rand() % 1000, rand() % 1000, rand() % 10, rand() % 10);
            tree.insert(i, box);
        }
        {
            std::fstream output("tab.txt", ios::out );//| ios::binary);
            tree.write_in_binary_file(output);

        }
        {
            std::fstream input("tab.txt", ios::in );//| ios::binary);
            treefromfile.read_from_binary_file(input);
        }
        auto box = createbox(rand() % 1000, rand() % 1000, rand() % 10, rand() % 10);
        auto origin_res = tree.find_objects_in_area(box);
        auto file_res = treefromfile.find_objects_in_area(box);
        sort(origin_res.begin(), origin_res.end());
        sort(file_res.begin(), file_res.end());
        if (origin_res == file_res) {
            cout << endl << "Result from original tree and from tree from file are equal";
        }
    }
    return 0;
}