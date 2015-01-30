#include <cudnn.h>
#include <iostream>
#include <stdint.h>
#include "operations.h"
#include "util.h"
#include "database.h"
#include "network.h"

using namespace std;

int main() {
	DataBase db("/home/marijnfs/dev/caffe/examples/cifar10/cifar10_train_leveldb");
	// db.floatify();

	double std(.005);
	double lr(.005);

	int n(1), c1(3), c2(50), h(32), w(32);
	int outc(10);

	Network network(TensorShape{n, c1, w, h});
	network.add_conv(c2, 5, 5);
	network.add_tanh();
	network.add_pool(4, 4);
	network.add_squash(10);
	network.add_softmax();
	network.finish();
	network.init_normal(0, std);

	//conv_operation1.init_normal(0, STD);
	//squash_operation.init_normal(0, STD);

	for (size_t e(0); e < 50; ++e) {
		Timer t;
		float err(0);
		int n_correct(0);

		for (size_t i(0); i < db.N; ++i) {
			//cout << i << endl;
			caffe::Datum datum = db.get_image(i);

			const float *img_data = datum.float_data().data();

			network.forward(img_data);
			network.backward(datum.label());
			network.update(lr);

			err += network.loss();
			n_correct += network.n_correct();
			
			if (i % 5000 == 0) {
				vector<float> ov = network.output().to_vector();
				cout << i << " " << datum.label() << " " << ov << " " << ov[datum.label()] << endl;
			}
		}
		cout << "elapsed: " << t.since() << " err: " << (err / db.N) << " correct: " << n_correct << "/" << db.N << endl;

	}
}

int main_nonetwork() {
	DataBase db("/home/marijnfs/dev/caffe/examples/cifar10/cifar10_train_leveldb");
	// db.floatify();

	double STD(.005);
	double lr(.005);

	int n(1), c1(3), c2(50), h(32), w(32);
	int outc(10);

	Tensor h1(n, c1, h, w);
	Tensor h2(n, c2, h, w);
	Tensor h3(n, c2, h, w);
	Tensor h4(n, c2, h/4, w/4);

	Tensor o1(n, outc, 1, 1); 
	Tensor o(n, outc, 1, 1);


	Tensor g_h1(n, c1, h, w);
	Tensor g_h2(n, c2, h, w);
	Tensor g_h3(n, c2, h, w);
	Tensor g_h4(n, c2, h/4, w/4);

	Tensor g_o1(n, outc, 1, 1); 
	Tensor g_o(n, outc, 1, 1); 


	ConvolutionOperation conv_operation1(c1, c2, 5, 5);
	TanhOperation tanh_operation;
	PoolingOperation pool_operation(4, 4);
	SquashOperation squash_operation(h4, 10);
	SoftmaxOperation softmax;
	SoftmaxLoss softmax_loss(n, outc);

	conv_operation1.init_normal(0, STD);
	squash_operation.init_normal(0, STD);

	for (size_t e(0); e < 50; ++e) {
		Timer t;
		float err(0);
		int n_correct(0);

		for (size_t i(0); i < db.N; ++i) {
			//cout << i << endl;
			caffe::Datum datum = db.get_image(i);

			const float *img_data = datum.float_data().data();

			h1.from_ptr(img_data);
			
			conv_operation1.forward(h1, h2);
			tanh_operation.forward(h2, h3);
			pool_operation.forward(h3, h4);

			squash_operation.forward(h4, o1);

			softmax.forward(o1, o);
			
			// cout << o.to_vector() << endl;
			softmax_loss.calculate_loss(o, datum.label(), g_o);
			if (isnan(o.to_vector()[0])) {
				//cout << "h1:" <<  h1.to_vector() << endl;
				//cout << "h2:" <<  h2.to_vector() << endl;
				//cout << "h3:" <<  h3.to_vector() << endl;
				
				cout << o1.to_vector() << endl;
				cout << o.to_vector() << endl;
			}
			
			softmax.backward(o1, o, g_o, g_o1);
			
			squash_operation.backward_weights(h4, g_o1);
			squash_operation.backward(h4, o1, g_o1, g_h4);

			pool_operation.backward(h3, h4, g_h4, g_h3);

			// cout << squash_operation.filter_bank_grad.to_vector() << endl;
			// return -1;
			
			tanh_operation.backward(h2, h3, g_h3, g_h2);
			conv_operation1.backward_weights(h1, g_h2);

			conv_operation1.update(lr);
			squash_operation.update(lr);

			err += softmax_loss.loss();
			n_correct += softmax_loss.n_correct();
			
			if (i % 5000 == 0) {
				vector<float> ov = o.to_vector();
				cout << i << " " << datum.label() << " " << ov << " " << ov[datum.label()] << endl;
				cout << conv_operation1.filter_bank.to_vector()[0] << endl;
				cout << squash_operation.filter_bank.to_vector()[0] << endl;

			}
			// conv_operation1.backward_input(g_h2, g_h1);

			// cout << h2.to_vector() << endl;
			// vector<float> vec2 = t2_map.to_vector();
			// vec2.resize(10);
			// cout << "t2out" << vec2 << endl << vec2 << endl;

			// Tensor out(n, w2, h2, 2);
		}
		cout << "elapsed: " << t.since() << " err: " << (err / db.N) << " correct: " << n_correct << "/" << db.N << endl;

	}
}

