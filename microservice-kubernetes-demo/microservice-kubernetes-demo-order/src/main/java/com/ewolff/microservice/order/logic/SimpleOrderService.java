package com.ewolff.microservice.order.logic;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;

import com.ewolff.microservice.order.clients.CatalogClient;
import com.ewolff.microservice.order.clients.Customer;
import com.ewolff.microservice.order.clients.CustomerClient;
import com.ewolff.microservice.order.clients.Item;

@Service
public class SimpleOrderService {

	private SimpleOrderRepository simpleOrderRepository;
	private CustomerClient customerClient;
	private CatalogClient catalogClient;

	@Autowired
	public SimpleOrderService(SimpleOrderRepository simpleOrderRepository,
			CustomerClient customerClient, CatalogClient catalogClient) {
		super();
		this.simpleOrderRepository = simpleOrderRepository;
		this.customerClient = customerClient;
		this.catalogClient = catalogClient;
	}

	public SimpleOrder createOrder(String customerId, long itemId, int quantity) {
		// Validate customer exists
		Customer customer = customerClient.getOne(customerId);
		if (customer == null) {
			throw new IllegalArgumentException("Customer does not exist!");
		}
		
		// Get item details
		Item item = catalogClient.getOne(itemId);
		if (item == null) {
			throw new IllegalArgumentException("Item does not exist!");
		}
		
		// Create simple order with denormalized data
		SimpleOrder order = new SimpleOrder(
			customer.getName(),
			item.getName(),
			quantity,
			item.getPrice() * quantity
		);
		
		return simpleOrderRepository.save(order);
	}

	public Iterable<SimpleOrder> findAll() {
		return simpleOrderRepository.findAll();
	}

	public SimpleOrder findById(long id) {
		return simpleOrderRepository.findById(id).orElse(null);
	}

	public void deleteById(long id) {
		simpleOrderRepository.deleteById(id);
	}
}
