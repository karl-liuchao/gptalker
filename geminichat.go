package main

import (
	"bufio"
	"context"
	"fmt"
	"log"
	"os"
	"strings"
	"io"

	"github.com/google/generative-ai-go/genai"
	"google.golang.org/api/option"
)

func main() {
	apiKey := os.Getenv("GOOGLE_API_KEY")
	if apiKey == "" {
		log.Fatal("Missing GOOGLE_API_KEY environment variable")
	}

	ctx := context.Background()
	client, err := genai.NewClient(ctx, option.WithAPIKey(apiKey))
	if err != nil {
		log.Fatal("Error creating client", err)
	}
	defer client.Close()

	model := client.GenerativeModel("gemini-pro") // You can change this to other models like "gemini-pro-vision"
    
    var conversation []*genai.Content


	fmt.Println("Welcome to the basic chat! Type 'exit' to end.")

	scanner := bufio.NewScanner(os.Stdin)
	for {
		fmt.Print("You: ")
		if !scanner.Scan() {
			break
		}

		userInput := scanner.Text()
		userInput = strings.TrimSpace(userInput)

		if strings.ToLower(userInput) == "exit" {
			fmt.Println("Exiting chat.")
			break
		}

        
        conversation = append(conversation, &genai.Content{
            Parts: []genai.Part{genai.Text(userInput)},
        })
        
		resp, err := model.GenerateContent(ctx, conversation...)
		if err != nil {
			fmt.Println("Error generating content:", err)
			continue // Go to next loop
		}

		if len(resp.Candidates) == 0 {
			fmt.Println("No response from the model.")
			continue
		}

		if len(resp.Candidates[0].Content.Parts) == 0 {
			fmt.Println("Empty response")
			continue
		}

		if text, ok := resp.Candidates[0].Content.Parts[0].(genai.Text); ok {
			fmt.Printf("AI: %s\n", string(text))
            conversation = append(conversation, &genai.Content{
                Parts: []genai.Part{genai.Text(string(text))},
            })
		} else {
			fmt.Println("Unexpected response format")
			continue
		}

		if err := scanner.Err(); err != nil {
			if err != io.EOF {
				log.Println("scanner error", err)
			}
			break
		}
	}
}
