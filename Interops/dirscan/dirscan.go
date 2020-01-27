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

type Frame struct {
	fname string
	cname string
	pname string
}

func main() {
	var frames []Frame
	var (
		classes = make(map[string]int)
		partitions = make(map[string]int)
	)
	var classPartitions = make(map[string]map[string]struct{})
	var (
		ctPath    = 0
		ctImage   = 0
		ctUnknown = 0
	)

	root := `C:\Users\aleoz\datasets\Linnaeus 5 256X256`
	err := filepath.Walk(root,
		func(path string, info os.FileInfo, err error) error {
			ctPath += 1
			dname, fname := filepath.Split(path)
			if filepath.Ext(fname) == ".jpg" {
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
				frames = append(frames, Frame{ filepath.Base(fname), cname, pname })
			} else if !info.IsDir() {
				ctUnknown += 1
			}
			return nil
		})
	if err != nil {
		panic(err)
	}
	// format: (.)(.)([^\2]+)(\2[^\2]+)* \1:TYPE \2:SEP ...:VALS
	fmt.Printf("h classes=%d parts=%d images=%d unknown=%d\n", len(classes), len(partitions), ctImage, ctUnknown)
	for cname := range classes {
		fmt.Printf("c+%d+%s\n", classes[cname], cname)
		for pname, _ := range classPartitions[cname] {
			fmt.Printf("p+%d+%s\n", partitions[pname], pname)
		}
	}
	for _, ff := range frames {
		fmt.Printf("f+%s+%s+%s\n", ff.fname, ff.cname, ff.pname)
	}
}
