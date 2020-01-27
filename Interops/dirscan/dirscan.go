package main

import (
	"fmt"
	"os"
	"path/filepath"
)

func incr (m *map[string]int, k string) {
	if val, ok := (*m)[k]; ok {
		(*m)[k] = val + 1
	} else {
		(*m)[k] = 1
	}
}

func main() {
	var files, dirs []string
	var (
		classes = make(map[string]int)
		partitions = make(map[string]int)
	)
	var classPartitions = make(map[string]map[string]struct{})
	var (
		ctPath    = 0
		ctImage   = 0
		ctDir     = 0
		ctUnknown = 0
	)

	root := `C:\Users\aleoz\datasets\Linnaeus 5 256X256`
	err := filepath.Walk(root,
		func(path string, info os.FileInfo, err error) error {
			ctPath += 1
			dname, fname := filepath.Split(path)
			if filepath.Ext(fname) == ".jpg" {
				files = append(files, path)
				ctImage += 1
				cname := filepath.Base(dname)
				incr(&classes, cname)
				pname := filepath.Clean(dname) // partition name
				incr(&partitions, pname)
				if partitionSet, ok := classPartitions[cname]; ok {
					if _, ok := partitionSet[pname]; !ok {
						classPartitions[cname][pname] = struct{}{}
					}
				} else {
					entry := make(map[string]struct{})
					entry[pname] = struct{}{}
					classPartitions[cname] = entry
				}
			} else if info.IsDir() {
				dirs = append(files, path)
				ctDir += 1
			} else {
				ctUnknown += 1
			}
			return nil
		})
	if err != nil {
		panic(err)
	}
	fmt.Printf("classes=%d parts=%d images=%d unknown=%d\n", len(classes), len(partitions), ctImage, ctUnknown)
	for cname := range classes {
		fmt.Println("c", classes[cname], cname)
		for pname, _ := range classPartitions[cname] {
			fmt.Println("p", partitions[pname], pname)
		}
	}
	//for _, file := range files {
	//	fmt.Println(ct_path, ctImage)
	//}
}
