package main

import (
	"crypto/rand"
	"fmt"
	"io"
	"io/ioutil"
	"log"
	"math"
	"mime"
	"net"
	"net/http"
	"os"
	"path/filepath"
	"strconv"
)

const bufferSize = 1024
const maxUploadSize = 2 * 1024 * 1024 // Max 2MB Transfer
const uploadPath = "./tmp"

func main() {
	http.HandleFunc("/upload", uploadFileHandler())

	fs := http.FileServer(http.Dir(uploadPath))
	http.Handle("/files/", http.StripPrefix("/files", fs))

	log.Println("Starting server at port :8080")

	log.Fatal(http.ListenAndServe(":8080", nil))
}

func uploadFileHandler() http.HandlerFunc {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		// validate file size
		r.Body = http.MaxBytesReader(w, r.Body, maxUploadSize)
		if err := r.ParseMultipartForm(maxUploadSize); err != nil {
			renderError(w, "FILE_TOO_BIG", http.StatusBadRequest)
			return
		}

		// parse and validate file and post parameters
		fileType := r.PostFormValue("type")
		file, _, err := r.FormFile("uploadFile")
		if err != nil {
			renderError(w, "INVALID_FILE", http.StatusBadRequest)
			return
		}
		defer file.Close()
		fileBytes, err := ioutil.ReadAll(file)
		if err != nil {
			renderError(w, "INVALID_FILE", http.StatusBadRequest)
			return
		}

		// check file type, detectcontenttype only needs the first 512 bytes
		filetype := http.DetectContentType(fileBytes)
		switch filetype {
		case "image/jpeg", "image/jpg":
		case "image/gif", "image/png":
			break
		default:
			renderError(w, "INVALID_FILE_TYPE", http.StatusBadRequest)
			return
		}
		fileName := randToken(12)
		fileEndings, err := mime.ExtensionsByType(filetype)
		if err != nil {
			renderError(w, "CANT_READ_FILE_TYPE", http.StatusInternalServerError)
			return
		}

		newPath := filepath.Join(uploadPath, fileName+fileEndings[0])
		log.Printf("FileType: %s, File: %s\n", fileType, newPath)

		// write file
		newFile, err := os.Create(newPath)
		if err != nil {
			renderError(w, "CANT_WRITE_FILE", http.StatusInternalServerError)
			return
		}
		defer newFile.Close() // idempotent, okay to call twice
		if _, err := newFile.Write(fileBytes); err != nil || newFile.Close() != nil {
			renderError(w, "CANT_WRITE_FILE", http.StatusInternalServerError)
			return
		}
		conn, err := net.Dial("tcp", "localhost:5000")
		if err != nil {
			panic(err)
		}
		defer conn.Close()
		log.Println("Connected to tcp server.")

		sendFile(conn, newPath)
		w.Write([]byte("SUCCESS"))
	})
}

func renderError(w http.ResponseWriter, message string, statusCode int) {
	w.WriteHeader(http.StatusBadRequest)
	w.Write([]byte(message))
}

func randToken(len int) string {
	b := make([]byte, len)
	rand.Read(b)
	return fmt.Sprintf("%x", b)
}

func sendFile(connection net.Conn, filePath string) {
	log.Println("A client has connected!")
	defer connection.Close()
	file, err := os.Open(filePath)
	if err != nil {
		log.Fatalln(err)
		return
	}

	fileInfo, err := file.Stat()
	if err != nil {
		log.Fatalln(err)
		return
	}

	fileSize := fillString(strconv.Itoa(int(math.Ceil(float64(fileInfo.Size())/float64(bufferSize)))*bufferSize), 16)
	connection.Write([]byte(fileSize))

	sendBuffer := make([]byte, bufferSize)

	if err != nil {
		log.Fatal(err)
		return
	}

	bytesSent := 0

	log.Println("Start sending file!")
	for {
		_, err = file.Read(sendBuffer)
		if err == io.EOF {
			break
		}
		connection.Write(sendBuffer)
		bytesSent += bufferSize
	}
	log.Printf("%dB sent\n", bytesSent)
	return
}

func fillString(retunString string, toLength int) string {
	for {
		lengtString := len(retunString)
		if lengtString < toLength {
			retunString = retunString + ":"
			continue
		}
		break
	}
	return retunString
}
