#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <limits>
#include <chrono>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;
using error = boost::system::error_code;

class item {
public:

	std::string nome;

	item(const std::string& nome) : nome(nome) {}

	std::string getNome() const { return nome; }

	virtual bool usar(int32_t& vida) { return false; }
};

class pocaoP : public item {
private:
	int32_t cura = 10;
public:
	pocaoP() : item("pocao"){}

	bool usar(int32_t& vida) override {
		std::cout << "A Poção da vida foi usada! Restaurando 10 de vida!\n";
		vida += cura;
		return true;
	}
};

class pocaoM : public item {
private:
	int32_t cura = 20;

public:
	pocaoM() : item("pocao1") {}

	bool usar(int32_t& vida) override{
		std::cout << "A Poção da vida foi usada! Restaurando 20 de vida!\n";
		vida += cura;
		return true;
	}
};

class Jogador {

private:
	int32_t level = 1;
	int32_t vida = 30;
	bool vivo = true;
	int32_t energia = 100;

	std::vector<std::shared_ptr<item>> inventario;

public:
	std::string nome;

	Jogador(std::string nome) {
		this->nome = nome;

		std::shared_ptr<item> pocao1 = std::make_shared<pocaoP>();
		std::shared_ptr<item> pocao2 = std::make_shared<pocaoM>();
		inventario.push_back(pocao1);
		inventario.push_back(pocao2);
	}

	void adicionar(std::shared_ptr<item> item) {
		inventario.push_back(item);
	}

	void usar(const std::string& itemUsar) {

		if (itemUsar == "nenhum") {
			std::cout << "nenhum item usado \n";
			return;
		}

		auto it = std::find_if(inventario.begin(), inventario.end(), [&](const std::shared_ptr<item> i) {
			return i->nome == itemUsar;
			});

		if (it != inventario.end()) {
			std::shared_ptr<item> encontrado = *it;

			if (encontrado->usar(vida)) {
				inventario.erase(it);
			}
		}
		else {
			std::cout << "nao foi possivel achar o item \n";
		}
	}

	void upar(int up) {
		std::cout << "Jogador upou " << up << " level \n";
		level += up;
	}

	void atacar(std::vector<std::shared_ptr<Jogador>>& j, const std::string& alvoI) {
		auto it = std::find_if(j.begin(), j.end(), [&](const std::shared_ptr<Jogador>& j) {return j->nome == alvoI;});

		if (it != j.end()) {
			auto alvo = *it;

			this->energia -= 10;

			if (this->energia > 10) {
				if (alvo->vida > 0) {
					alvo->vida -= 10;
					std::cout << nome << " atacou " << alvoI << "\n";
					std::cout << "a vida do jogador " << alvoI << " está em " << alvo->vida << "\n";
				}
			}
			else {
				std::cout << "energia insuficiente \n";
				return;
			}

			if (alvo->vida <= 0) {
				alvo->vivo = false;
				delet(j, alvoI);
			}
		}
		else {
			std::cout << "Jogador não encontrado \n";
		}
	}

	void morto() {
		if (vivo) {
			std::cout << "O jogador está vivo \n";
		}
		else if (!vivo) {
			std::cout << "O jogador morreu \n";
		}
	}

	void delet(std::vector<std::shared_ptr<Jogador>> j,const std::string& nome) {
		auto it = std::find_if(j.begin(), j.end(), [&](const std::shared_ptr<Jogador> j) {
			return j->nome == nome;
			});

		if (it != j.end()) {
			j.erase(it);
			std::cout << "jogador eliminado \n";
		}
	}

	void regenerar() {

		if (energia < 100) {
			std::cout << "recuperou 5 de energia \n";
			energia += 5;
		}
		else if(energia >= 100){
			std::cout << "energia está cheia \n";
			energia = 100;
		}
	}
};

class Servidor {
private:
	tcp::acceptor aceitar;
	std::vector<std::shared_ptr<Jogador>> jogadores;

public:
	Servidor(boost::asio::io_context& context) : aceitar(context, tcp::endpoint(tcp::v4(), 8080)) {
		jogadores.push_back(std::make_shared<Jogador>("Rogerio"));
		jogadores.push_back(std::make_shared<Jogador>("Ronaldo"));
	}

	void iniciar() {
		std::cout << "servidor rodando na porta 8080 \n";
		while (true) {
			tcp::socket sock(aceitar.get_executor());
			aceitar.accept(sock);

			std::thread(&Servidor::handle, this, std::move(sock)).detach();
		}
	}

private:

	void handle(tcp::socket sock) {
		std::shared_ptr<Jogador> j = jogadores[0];

		while (true) {
			try {
				std::array<char, 1024>data;

				std::cout << "escolha um movimento \n";
				boost::asio::write(sock, boost::asio::buffer(
					"escolha um movimento \n1 para atacar \n2 para sair\n3 para descansar\n"));
				size_t tamanho = sock.read_some(boost::asio::buffer(data));
				std::string conversao(data.data(), tamanho);

				int32_t opcao = std::stoi(conversao);
		
				if (opcao == 1) {
					handle_atacar(sock,j);
				}
				else if (opcao == 2) {
					handle_usar(sock, j);
				}
				else if (opcao == 3) {
					descansar(j);
				}
				else if (opcao == 4) {
					break;
				}
			}
			catch (const error& e) {
				std::cerr << "ocorreu um erro " << e.what() << "\n";
				break;
			}
			catch (const std::exception& e) {
				std::cerr << "erro inesperado " << e.what() << "\n";
				break;
			}

		}

		sock.close();
		std::cout << "jogador desconectado \n";
	}

	void handle_atacar(tcp::socket& sock, std::shared_ptr<Jogador> j) {

		std::array<char, 1024>data;

		std::string message = "Coloque o nome da pessoa que vc quer atacar \n";
		boost::asio::write(sock, boost::asio::buffer(message));
		size_t tamanho = sock.read_some(boost::asio::buffer(data));

		std::string nomes(data.data(), tamanho);

		j->atacar(jogadores, nomes);
	}

	void handle_usar(tcp::socket& sock, std::shared_ptr<Jogador> j) {
		std::array<char, 1024> data;

		std::string message = "Escolha o nome do item para usar \n";
		boost::asio::write(sock, boost::asio::buffer(message));

		size_t tamanho = sock.read_some(boost::asio::buffer(data));
		std::string nomeItem(data.data(), tamanho);

		j->usar(nomeItem);
	}

	void descansar(std::shared_ptr<Jogador> j) {
		j->regenerar();
	}
};

int main() {

	boost::asio::io_context context;

	Servidor servidor(context);
	servidor.iniciar();

	
}